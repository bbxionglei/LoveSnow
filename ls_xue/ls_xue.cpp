// ls_xue.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <iostream>
#include <ls_base/ls.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.hpp>
#include <signal.h>
#include <assert.h>
#include <assert.h>
#include <ls_base/socket_server.h>

//Lua_lib			lua源码
//ls_base			提供基本API
//ls_gateway		模块:网关
//ls_load_script	模块:加载脚本
//ls_script_lib		提供脚本接口
//ls_xue			主程序
//Lua_base			调用ls_script_lib 封装lua接口
//Lua_service		调用Lua_base封装成服务，被ls_load_script加载，其中有一个lua负责加载业务脚本
//examples			调用Lua_base完成业务逻辑，被Lua_service加载



struct ls_env {
	struct spinlock lock;
};

static int
optint(const char* key, int opt) {
	const char* str = ls_getenv(key);
	if (str == NULL) {
		char tmp[20];
		sprintf(tmp, "%d", opt);
		ls_setenv(key, tmp);
		return opt;
	}
	return strtol(str, NULL, 10);
}

static int
optboolean(const char* key, int opt) {
	const char* str = ls_getenv(key);
	if (str == NULL) {
		ls_setenv(key, opt ? "true" : "false");
		return opt;
	}
	return strcmp(str, "true") == 0;
}

static const char*
optstring(const char* key, const char* opt) {
	const char* str = ls_getenv(key);
	if (str == NULL) {
		if (opt) {
			ls_setenv(key, opt);
			opt = ls_getenv(key);
		}
		return opt;
	}
	return str;
}

static void
_init_env(lua_State * L) {
	lua_pushnil(L);  /* first key */
	while (lua_next(L, -2) != 0) {
		int keyt = lua_type(L, -2);
		if (keyt != LUA_TSTRING) {
			fprintf(stderr, "Invalid config table\n");
			exit(1);
		}
		const char* key = lua_tostring(L, -2);
		if (lua_type(L, -1) == LUA_TBOOLEAN) {
			int b = lua_toboolean(L, -1);
			ls_setenv(key, b ? "true" : "false");
		}
		else {
			const char* value = lua_tostring(L, -1);
			if (value == NULL) {
				fprintf(stderr, "Invalid config table key = %s\n", key);
				exit(1);
			}
			ls_setenv(key, value);
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

int sigign() {
#ifndef _WIN32
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, 0);
#endif
	return 0;
}


static const char* load_config = "\
	local result = {}\n\
	local function getenv(name) return assert(os.getenv(name), [[os.getenv() failed: ]] .. name) end\n\
	local sep = package.config:sub(1,1)\n\
	local current_path = [[.]]..sep\n\
	local function include(filename)\n\
		local last_path = current_path\n\
		local path, name = filename:match([[(.*]]..sep..[[)(.*)$]])\n\
		if path then\n\
			if path:sub(1,1) == sep then	-- root\n\
				current_path = path\n\
			else\n\
				current_path = current_path .. path\n\
			end\n\
		else\n\
			name = filename\n\
		end\n\
		local f = assert(io.open(current_path .. name))\n\
		local code = assert(f:read [[*a]])\n\
		code = string.gsub(code, [[%$([%w_%d]+)]], getenv)\n\
		f:close()\n\
		assert(load(code,[[@]]..filename,[[t]],result))()\n\
		current_path = last_path\n\
	end\n\
	setmetatable(result, { __index = { include = include } })\n\
	local config_name = ...\n\
	include(config_name)\n\
	setmetatable(result, nil)\n\
	return result\n\
";


int main(int argc, char* argv[])
{
#ifdef _WIN32
	printf("hello _WIN32\n");
#else
	printf("hello not _WIN32\n");
#endif


	const char* config_file = NULL;
	if (argc > 1) {
		config_file = argv[1];
	}
	else {
		fprintf(stderr, "usage: ls configfilename\n");
		return 1;
	}

	luaS_initshr();
	ls_globalinit();
	ls_env_init();

	sigign();

	struct ls_config config;

	struct lua_State* L = luaL_newstate();
	luaL_openlibs(L);	// link lua lib

	int err = luaL_loadbufferx(L, load_config, strlen(load_config), "=[ls config]", "t");
	assert(err == LUA_OK);
	lua_pushstring(L, config_file);

	err = lua_pcall(L, 1, 1, 0);
	if (err) {
		fprintf(stderr, "%s\n", lua_tostring(L, -1));
		lua_close(L);
		return 1;
	}
	_init_env(L);

	config.thread = optint("thread", 8);
	config.module_path = optstring("cpath", "./cservice/?.so");
	config.harbor = optint("harbor", 1);
	config.bootstrap = optstring("bootstrap", "ls_load_script bootstrap");
	config.daemon = optstring("daemon", NULL);
	config.logger = optstring("logger", NULL);
	config.logservice = optstring("logservice", "ls_logger");
	config.profile = optboolean("profile", 1);
	
	printf("config.module_path=%s\n", config.module_path);

	lua_close(L);

	ls_start(&config);

	ls_globalexit();
	luaS_exitshr();

	return 0;
}
