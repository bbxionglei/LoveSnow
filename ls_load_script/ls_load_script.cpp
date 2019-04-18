
#include "ls_load_script.h"
#include <ls_base/ls.h>
#include <lua.hpp>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MEMORY_WARNING_REPORT (1024 * 1024 * 32)

struct ls_load_script {
	lua_State* L;
	struct ls_context* ctx;
	size_t mem;
	size_t mem_report;
	size_t mem_limit;
};

// LUA_CACHELIB may defined in patched lua for shared proto
#ifdef LUA_CACHELIB

#define codecache luaopen_cache

#else

static int
cleardummy(lua_State* L) {
	return 0;
}

static int
codecache(lua_State* L) {
	luaL_Reg l[] = {
		{ "clear", cleardummy },
		{ "mode", cleardummy },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	lua_getglobal(L, "loadfile");
	lua_setfield(L, -2, "loadfile");
	return 1;
}

#endif

static int
traceback(lua_State* L) {
	const char* msg = lua_tostring(L, 1);
	if (msg)
		luaL_traceback(L, L, msg, 1);
	else {
		lua_pushliteral(L, "(no error message)");
	}
	return 1;
}

static void
report_launcher_error(struct ls_context* ctx) {
	// sizeof "ERROR" == 5
	char szError[] = "ERROR";
	ls_sendname(ctx, 0, ".launcher", PTYPE_TEXT, 0, szError, 5);
}

static const char*
optstring(struct ls_context* ctx, const char* key, const char* str) {
	const char* ret = ls_command(ctx, "GETENV", key);
	if (ret == NULL) {
		return str;
	}
	return ret;
}

static int
init_cb(struct ls_load_script* l, struct ls_context* ctx, const char* args, size_t sz) {
	lua_State* L = l->L;
	l->ctx = ctx;
	lua_gc(L, LUA_GCSTOP, 0);
	lua_pushboolean(L, 1);  /* signal for libraries to ignore env. vars. *///#stack == 1
	lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");////#stack == 0
	luaL_openlibs(L);
	lua_pushlightuserdata(L, ctx);////#stack == 1
	lua_setfield(L, LUA_REGISTRYINDEX, "ls_context");//#stack == 0
	luaL_requiref(L, "lovesnow.codecache", codecache, 0);//#stack == 1
	lua_pop(L, 1);//#stack == 0

	const char* path = optstring(ctx, "lua_path", "./lualib/?.lua;./lualib/?/init.lua");
	lua_pushstring(L, path);//#stack == 1
	lua_setglobal(L, "LUA_PATH");//#stack == 0
	const char* cpath = optstring(ctx, "lua_cpath", "./luaclib/?.so");
	lua_pushstring(L, cpath);//#stack == 1
	lua_setglobal(L, "LUA_CPATH");//#stack == 0
	const char* service = optstring(ctx, "luaservice", "./service/?.lua");
	lua_pushstring(L, service);//#stack == 1
	lua_setglobal(L, "LUA_SERVICE");//#stack == 0
	const char* preload = ls_command(ctx, "GETENV", "preload");
	lua_pushstring(L, preload);//#stack == 1
	lua_setglobal(L, "LUA_PRELOAD");//#stack == 0

	lua_pushcfunction(L, traceback);//#stack == 1
	assert(lua_gettop(L) == 1);//哈哈，校验lua栈有没有问题

	const char* loader = optstring(ctx, "lualoader", "./lualib/loader.lua");

	int r = luaL_loadfile(L, loader); // 加载loader模块代码
	if (r != LUA_OK) {
		ls_error(ctx, "Can't load %s : %s", loader, lua_tostring(L, -1));
		report_launcher_error(ctx);
		return 1;
	}
	lua_pushlstring(L, args, sz);//args 第一次调用这个回调默认参数是bootstrap,消息是ls_load_script_init的时候发送的也应该是整个系统的第一个消息
	// 把服务名等参数传入，执行loader模块代码，实际上是通过loader加载和执行服务代码
	r = lua_pcall(L, 1, 0, 1);
	if (r != LUA_OK) {
		ls_error(ctx, "lua loader error : %s", lua_tostring(L, -1));
		report_launcher_error(ctx);
		return 1;
	}
	lua_settop(L, 0);
	if (lua_getfield(L, LUA_REGISTRYINDEX, "memlimit") == LUA_TNUMBER) {
		size_t limit = lua_tointeger(L, -1);
		l->mem_limit = limit;
		ls_error(ctx, "Set memory limit to %.2f M", (float)limit / (1024 * 1024));
		lua_pushnil(L);
		lua_setfield(L, LUA_REGISTRYINDEX, "memlimit");
	}
	lua_pop(L, 1);

	lua_gc(L, LUA_GCRESTART, 0);

	return 0;
}

static int
launch_cb(struct ls_context* context, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz) {
	assert(type == 0 && session == 0);
	struct ls_load_script* l = (struct ls_load_script*)ud;
	ls_callback(context, NULL, NULL);//bootstrap程序只需要处理一次就够了，所以这里关闭回调
	int err = init_cb(l, context, (const char*)msg, sz);// msg 仍然是bootstrap
	if (err) {
		ls_command(context, "EXIT", NULL);
	}

	return 0;
}

int
ls_load_script_init(void* l_, struct ls_context* ctx, const char* args) {
	struct ls_load_script* l = (struct ls_load_script*)l_;
	int sz = strlen(args);//此处默认是 bootstrap 因为配置默认是 bootstrap = "ls_load_script bootstrap"	-- The service for bootstrap
	char* tmp = (char*)ls_malloc(sz);
	memcpy(tmp, args, sz);
	ls_callback(ctx, l, launch_cb);
	const char* self = ls_command(ctx, "REG", NULL);
	uint32_t handle_id = strtoul(self + 1, NULL, 16);
	/* it must be first message
	 * 把参数当作消息内容发给这个服务，就是 ls.newservice(name, ...) 后面的 ...
	 * 目的是驱动服务完成初始化，后面会讲到，ls服务是消息驱动。
	 * 调用这个函数的时候，工作线程还没有工作，消息还不会处理,
	 * 所以下面ls_send的消息要等系统开始处理消息的时候才会进入launch_cb
	 */
	ls_send(ctx, 0, handle_id, PTYPE_TAG_DONTCOPY, 0, tmp, sz);
	return 0;
}

static void*
lalloc(void* ud, void* ptr, size_t osize, size_t nsize) {
	struct ls_load_script* l = (struct ls_load_script*)ud;
	size_t mem = l->mem;
	l->mem += nsize;
	if (ptr)
		l->mem -= osize;
	if (l->mem_limit != 0 && l->mem > l->mem_limit) {
		if (ptr == NULL || nsize > osize) {
			l->mem = mem;
			return NULL;
		}
	}
	if (l->mem > l->mem_report) {
		l->mem_report *= 2;
		ls_error(l->ctx, "Memory warning %.2f M", (float)l->mem / (1024 * 1024));
	}
	return ls_lalloc(ptr, osize, nsize);
}

void*
ls_load_script_create(void) {
	struct ls_load_script* l = (struct ls_load_script*)ls_malloc(sizeof(*l));
	memset(l, 0, sizeof(*l));
	l->mem_report = MEMORY_WARNING_REPORT;
	l->mem_limit = 0;
	l->L = lua_newstate(lalloc, l);
	return l;
}

void
ls_load_script_release(void* l_) {
	struct ls_load_script* l = (struct ls_load_script*)l_;
	lua_close(l->L);
	ls_free(l);
}

void
ls_load_script_signal(void* l_, int signal) {
	struct ls_load_script* l = (struct ls_load_script*)l_;
	ls_error(l->ctx, "recv a signal %d", signal);
	if (signal == 0) {
#ifdef lua_checksig
		// If our lua support signal (modified lua version by ls), trigger it.
		ls_sig_L = l->L;
#endif
	}
	else if (signal == 1) {
		ls_error(l->ctx, "Current Memory %.3fK", (float)l->mem / 1024);
	}
}
