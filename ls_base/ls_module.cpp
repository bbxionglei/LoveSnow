#include "ls.h"

#include "ls_module.h"
#include "spinlock.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

struct model_st {
	char name[64];
	ls_dl_create create;
	ls_dl_init init;
	ls_dl_release release;
	ls_dl_signal signal;
};

#define MAX_MODULE_TYPE 32

struct modules {
	int count;
	struct spinlock lock;
	const char* path;
	struct ls_module m[MAX_MODULE_TYPE];
};

static struct modules* M = NULL;

static void*
_try_open(struct modules* m, const char* name) {
	const char* l;
	const char* path = m->path;
	size_t path_size = strlen(path);
	size_t name_size = strlen(name);

	int sz = path_size + name_size + 5;
	//search path
#ifdef _WIN32
	HMODULE dl = NULL;
#else
	void* dl = NULL;
#endif
	char* tmp = (char*)ls_malloc(sz);
	do
	{
		memset(tmp, 0, sz);
		while (*path == ';') path++;
		if (*path == '\0') break;
		l = strchr(path, ';');
		if (l == NULL) l = path + strlen(path);
		int len = l - path;
		int i;
		for (i = 0; path[i] != '?' && i < len; i++) {
			tmp[i] = path[i];
		}
		memcpy(tmp + i, name, name_size);
		if (path[i] == '?') {
			strncpy(tmp + i + name_size, path + i + 1, len - i - 1);
		}
		else {
			fprintf(stderr, "Invalid C service path\n");
			exit(1);
		}
#ifdef _WIN32
		strcpy(tmp + strlen(tmp), ".dll");
		dl = LoadLibraryA(tmp);
#else
		strcpy(tmp + strlen(tmp), ".so");
		//dl = dlopen(tmp, RTLD_NOW | RTLD_GLOBAL);
		dl = dlopen(tmp, RTLD_NOW | RTLD_GLOBAL);
#endif
		path = l;
	} while (dl == NULL);
	ls_free(tmp);
	if (dl == NULL) {
#ifdef _WIN32
		fprintf(stderr, "LoadLibraryA try open %s failed\n", name);
#else
		fprintf(stderr, "dlopen try open %s failed : %s\n", name, dlerror());
#endif
	}
	return dl;
}

static struct ls_module*
_query(const char* name) {
	int i;
	for (i = 0; i < M->count; i++) {
		if (strcmp(M->m[i].name, name) == 0) {
			return &M->m[i];
		}
	}
	return NULL;
}

static void*
get_api(struct ls_module* mod, const char* api_name) {
	size_t name_size = strlen(mod->name);
	size_t api_size = strlen(api_name);
	char* tmp = (char*)ls_malloc(name_size + api_size + 1);
	memcpy(tmp, mod->name, name_size);
	memcpy(tmp + name_size, api_name, api_size + 1);
	char* ptr = strrchr(tmp, '.');
	if (ptr == NULL) {
		ptr = tmp;
	}
	else {
		ptr = ptr + 1;
	}
	void* ret = NULL;
#ifdef _WIN32
	ret = GetProcAddress((HMODULE)mod->module, ptr);
#else
	//dlsym 通过拼接的字符串找到对应的函数 snlua_create
	ret = dlsym(mod->module, ptr);
#endif
	ls_free(tmp);
	return ret;
}

static int
open_sym(struct ls_module* mod) {
	mod->create = (ls_dl_create)get_api(mod, "_create");
	mod->init = (ls_dl_init)get_api(mod, "_init");
	mod->release = (ls_dl_release)get_api(mod, "_release");
	mod->signal = (ls_dl_signal)get_api(mod, "_signal");

	return mod->init == NULL;
}

struct ls_module*
	ls_module_query(const char* name) {
	struct ls_module* result = _query(name);
	if (result)
		return result;

	SPIN_LOCK(M);

	result = _query(name); // double check

	if (result == NULL && M->count < MAX_MODULE_TYPE) {
		int index = M->count;
		void* dl = _try_open(M, name);
		if (dl) {
			M->m[index].name = name;
			M->m[index].module = dl;

			//open_sym 关联 _create _init _release _signal 到 ls_module 的 create init release signal
			//如：snlua_create logger_create harbor_create gate_create
			if (open_sym(&M->m[index]) == 0) {
				M->m[index].name = ls_strdup(name);
				M->count++;
				result = &M->m[index];
			}
		}
	}

	SPIN_UNLOCK(M);

	return result;
}

void
ls_module_insert(struct ls_module* mod) {
	SPIN_LOCK(M);

	struct ls_module* m = _query(mod->name);
	assert(m == NULL && M->count < MAX_MODULE_TYPE);
	int index = M->count;
	M->m[index] = *mod;
	++M->count;

	SPIN_UNLOCK(M);
}

void*
ls_module_instance_create(struct ls_module* m) {
	if (m->create) {
		return m->create();//调用模块的 _create 如snlua_create
	}
	else {
		return (void*)(intptr_t)(~0);
	}
}

int
ls_module_instance_init(struct ls_module* m, void* inst, struct ls_context* ctx, const char* parm) {
	return m->init(inst, ctx, parm);//调用模块的 _init 如snlua_init
}

void
ls_module_instance_release(struct ls_module* m, void* inst) {
	if (m->release) {
		m->release(inst);//调用模块的 _release 如snlua_release
	}
}

void
ls_module_instance_signal(struct ls_module* m, void* inst, int signal) {
	if (m->signal) {
		m->signal(inst, signal);//调用模块的 _signal 如snlua_signal
	}
}

void
ls_module_init(const char* path) {
	struct modules* m = (struct modules*)ls_malloc(sizeof(*m));
	m->count = 0;
	m->path = ls_strdup(path);

	SPIN_INIT(m);

	M = m;
}
