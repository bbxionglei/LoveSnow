#include "ls_module.h"
static void*
_try_open(const char * path, const char* name) {
	const char* l;
	size_t path_size = strlen(path);
	size_t name_size = strlen(name);

	int sz = path_size + name_size;
	//search path
	HMODULE dl = nullptr;
	char* tmp = new char[sz];
	do
	{
		memset(tmp, 0, sz);
		while (*path == ';') path++;
		if (*path == '\0') break;
		l = strchr(path, ';');
		if (l == nullptr) l = path + strlen(path);
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
		dl = LoadLibraryA(tmp);
		path = l;
	} while (dl == nullptr);
	if (dl == nullptr) {
		fprintf(stderr, "LoadLibraryA try open %s failed\n", name);
		exit(1);
	}
	return dl;
}

static void*
get_api(struct ls_module* mod, const char* api_name) {
	size_t name_size = strlen(mod->name);
	size_t api_size = strlen(api_name);
	char* tmp = new char[name_size + api_size + 1];
	memcpy(tmp, mod->name, name_size);
	memcpy(tmp + name_size, api_name, api_size + 1);
	char* ptr = strrchr(tmp, '.');
	if (ptr == nullptr) {
		ptr = tmp;
	}
	else {
		ptr = ptr + 1;
	}
	void* ret = nullptr;
	ret = GetProcAddress((HMODULE)mod->module, ptr);
	return ret;
}
static int
open_sym(struct ls_module* mod) {

	mod->create = (ls_dl_create)get_api(mod, "_create");
	mod->init = (ls_dl_init)get_api(mod, "_init");
	mod->release = (ls_dl_release)get_api(mod, "_release");
	mod->signal = (ls_dl_signal)get_api(mod, "_signal");

	return mod->init == nullptr;
}
ls_module* ls_module_query(const char* path, const char* name)
{
	struct ls_module* M = new struct ls_module;
	void* dl = _try_open(path, name);
	if (dl) {
		M->name = name;
		M->module = dl;

		if (open_sym(M) == 0) {
			return M;
		}
	}
	return nullptr;
}