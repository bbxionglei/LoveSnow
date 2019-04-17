
#include <ls_base/ls.h>
//TODO #include "lua-seri.h"
#include "ls_script_lib.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"

#include <lua.hpp>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <time.h>
#include <chrono>
#include <ratio>

#if defined(__APPLE__)
#include <sys/time.h>
#endif


// return nsec
static int64_t
get_time() {
	std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<uint64_t, std::nano>> now
		= std::chrono::time_point_cast<std::chrono::duration<uint64_t, std::nano>>(std::chrono::system_clock::now());
	return now.time_since_epoch().count();
}

struct snlua {
	lua_State* L;
	struct ls_context* ctx;
	const char* preload;
};


static int
traceback(lua_State* L) {
	const char* msg = (const char*)lua_tostring(L, 1);
	if (msg)
		luaL_traceback(L, L, msg, 1);
	else {
		lua_pushliteral(L, "(no error message)");
	}
	return 1;
}

static int
_cb(struct ls_context* context, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz) {
	lua_State* L = (lua_State*)ud;
	int trace = 1;
	int r;
	int top = lua_gettop(L);
	if (top == 0) {
		lua_pushcfunction(L, traceback);
		lua_rawgetp(L, LUA_REGISTRYINDEX, _cb);
	}
	else {
		assert(top == 2);
	}
	lua_pushvalue(L, 2);

	lua_pushinteger(L, type);
	lua_pushlightuserdata(L, (void*)msg);
	lua_pushinteger(L, sz);
	lua_pushinteger(L, session);
	lua_pushinteger(L, source);

	r = lua_pcall(L, 5, 0, trace);

	if (r == LUA_OK) {
		return 0;
	}
	const char* self = ls_command(context, "REG", NULL);
	switch (r) {
	case LUA_ERRRUN:
		ls_error(context, "lua call [%x to %s : %d msgsz = %d] error : " KRED "%s" KNRM, source, self, session, sz, lua_tostring(L, -1));
		break;
	case LUA_ERRMEM:
		ls_error(context, "lua memory error : [%x to %s : %d]", source, self, session);
		break;
	case LUA_ERRERR:
		ls_error(context, "lua error in error : [%x to %s : %d]", source, self, session);
		break;
	case LUA_ERRGCMM:
		ls_error(context, "lua gc error : [%x to %s : %d]", source, self, session);
		break;
	};

	lua_pop(L, 1);

	return 0;
}

static int
forward_cb(struct ls_context* context, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz) {
	_cb(context, ud, type, session, source, msg, sz);
	// don't delete msg in forward mode.
	return 1;
}

static int
lcallback(lua_State * L) {
	struct ls_context* context = (struct ls_context*)lua_touserdata(L, lua_upvalueindex(1));
	int forward = lua_toboolean(L, 2);
	luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_settop(L, 1);
	lua_rawsetp(L, LUA_REGISTRYINDEX, _cb);

	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
	lua_State* gL = lua_tothread(L, -1);

	if (forward) {
		ls_callback(context, gL, forward_cb);
	}
	else {
		ls_callback(context, gL, _cb);
	}

	return 0;
}

static int
lcommand(lua_State * L) {
	struct ls_context* context = (struct ls_context*)lua_touserdata(L, lua_upvalueindex(1));
	const char* cmd = luaL_checkstring(L, 1);
	const char* result;
	const char* parm = NULL;
	if (lua_gettop(L) == 2) {
		parm = luaL_checkstring(L, 2);
	}

	result = ls_command(context, cmd, parm);
	if (result) {
		lua_pushstring(L, result);
		return 1;
	}
	return 0;
}

static int
laddresscommand(lua_State * L) {
	struct ls_context* context = (struct ls_context*)lua_touserdata(L, lua_upvalueindex(1));
	const char* cmd = luaL_checkstring(L, 1);
	const char* result;
	const char* parm = NULL;
	if (lua_gettop(L) == 2) {
		parm = luaL_checkstring(L, 2);
	}
	result = ls_command(context, cmd, parm);
	if (result && result[0] == ':') {
		int i;
		uint32_t addr = 0;
		for (i = 1; result[i]; i++) {
			int c = result[i];
			if (c >= '0' && c <= '9') {
				c = c - '0';
			}
			else if (c >= 'a' && c <= 'f') {
				c = c - 'a' + 10;
			}
			else if (c >= 'A' && c <= 'F') {
				c = c - 'A' + 10;
			}
			else {
				return 0;
			}
			addr = addr * 16 + c;
		}
		lua_pushinteger(L, addr);
		return 1;
	}
	return 0;
}

static int
lintcommand(lua_State * L) {
	struct ls_context* context = (struct ls_context*)lua_touserdata(L, lua_upvalueindex(1));
	const char* cmd = luaL_checkstring(L, 1);
	const char* result;
	const char* parm = NULL;
	char tmp[64];	// for integer parm
	if (lua_gettop(L) == 2) {
		if (lua_isnumber(L, 2)) {
			int32_t n = (int32_t)luaL_checkinteger(L, 2);
			sprintf(tmp, "%d", n);
			parm = tmp;
		}
		else {
			parm = luaL_checkstring(L, 2);
		}
	}

	result = ls_command(context, cmd, parm);
	if (result) {
		char* endptr = NULL;
		lua_Integer r = strtoll(result, &endptr, 0);
		if (endptr == NULL || *endptr != '\0') {
			// may be real number
			double n = strtod(result, &endptr);
			if (endptr == NULL || *endptr != '\0') {
				return luaL_error(L, "Invalid result %s", result);
			}
			else {
				lua_pushnumber(L, n);
			}
		}
		else {
			lua_pushinteger(L, r);
		}
		return 1;
	}
	return 0;
}

static int
lgenid(lua_State * L) {
	struct ls_context* context = (struct ls_context*)lua_touserdata(L, lua_upvalueindex(1));
	int session = ls_send(context, 0, 0, PTYPE_TAG_ALLOCSESSION, 0, NULL, 0);
	lua_pushinteger(L, session);
	return 1;
}

static const char*
get_dest_string(lua_State * L, int index) {
	const char* dest_string = lua_tostring(L, index);
	if (dest_string == NULL) {
		luaL_error(L, "dest address type (%s) must be a string or number.", lua_typename(L, lua_type(L, index)));
	}
	return dest_string;
}

static int
send_message(lua_State * L, int source, int idx_type) {
	struct ls_context* context = (struct ls_context*)lua_touserdata(L, lua_upvalueindex(1));
	uint32_t dest = (uint32_t)lua_tointeger(L, 1);
	const char* dest_string = NULL;
	if (dest == 0) {
		if (lua_type(L, 1) == LUA_TNUMBER) {
			return luaL_error(L, "Invalid service address 0");
		}
		dest_string = get_dest_string(L, 1);
	}

	int type = luaL_checkinteger(L, idx_type + 0);
	int session = 0;
	if (lua_isnil(L, idx_type + 1)) {
		type |= PTYPE_TAG_ALLOCSESSION;
	}
	else {
		session = luaL_checkinteger(L, idx_type + 1);
	}

	int mtype = lua_type(L, idx_type + 2);
	switch (mtype) {
	case LUA_TSTRING: {
		size_t len = 0;
		void* msg = (void*)lua_tolstring(L, idx_type + 2, &len);
		if (len == 0) {
			msg = NULL;
		}
		if (dest_string) {
			session = ls_sendname(context, source, dest_string, type, session, msg, len);
		}
		else {
			session = ls_send(context, source, dest, type, session, msg, len);
		}
		break;
	}
	case LUA_TLIGHTUSERDATA: {
		void* msg = lua_touserdata(L, idx_type + 2);
		int size = luaL_checkinteger(L, idx_type + 3);
		if (dest_string) {
			session = ls_sendname(context, source, dest_string, type | PTYPE_TAG_DONTCOPY, session, msg, size);
		}
		else {
			session = ls_send(context, source, dest, type | PTYPE_TAG_DONTCOPY, session, msg, size);
		}
		break;
	}
	default:
		luaL_error(L, "invalid param %s", lua_typename(L, lua_type(L, idx_type + 2)));
	}
	if (session < 0) {
		if (session == -2) {
			// package is too large
			lua_pushboolean(L, 0);
			return 1;
		}
		// send to invalid address
		// todo: maybe throw an error would be better
		return 0;
	}
	lua_pushinteger(L, session);
	return 1;
}

/*
	uint32 address
	 string address
	integer type
	integer session
	string message
	 lightuserdata message_ptr
	 integer len
 */
static int
lsend(lua_State * L) {
	return send_message(L, 0, 2);
}

/*
	uint32 address
	 string address
	integer source_address
	integer type
	integer session
	string message
	 lightuserdata message_ptr
	 integer len
 */
static int
lredirect(lua_State * L) {
	uint32_t source = (uint32_t)luaL_checkinteger(L, 2);
	return send_message(L, source, 3);
}

static int
lerror(lua_State * L) {
	struct ls_context* context = (struct ls_context*)lua_touserdata(L, lua_upvalueindex(1));
	int n = lua_gettop(L);
	if (n <= 1) {
		lua_settop(L, 1);
		const char* s = luaL_tolstring(L, 1, NULL);
		ls_error(context, "%s", s);
		return 0;
	}
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	int i;
	for (i = 1; i <= n; i++) {
		luaL_tolstring(L, i, NULL);
		luaL_addvalue(&b);
		if (i < n) {
			luaL_addchar(&b, ' ');
		}
	}
	luaL_pushresult(&b);
	ls_error(context, "%s", lua_tostring(L, -1));
	return 0;
}

static int
ltostring(lua_State * L) {
	if (lua_isnoneornil(L, 1)) {
		return 0;
	}
	char* msg = (char*)lua_touserdata(L, 1);
	int sz = luaL_checkinteger(L, 2);
	lua_pushlstring(L, msg, sz);
	return 1;
}

static int
lharbor(lua_State * L) {
	struct ls_context* context = (struct ls_context*)lua_touserdata(L, lua_upvalueindex(1));
	uint32_t handle = (uint32_t)luaL_checkinteger(L, 1);
	int harbor = 0;
	int remote = ls_isremote(context, handle, &harbor);
	lua_pushinteger(L, harbor);
	lua_pushboolean(L, remote);

	return 2;
}

static int
lpackstring(lua_State * L) {
	//TODO luaseri_pack(L);
	char* str = (char*)lua_touserdata(L, -2);
	int sz = lua_tointeger(L, -1);
	lua_pushlstring(L, str, sz);
	ls_free(str);
	return 1;
}

static int
ltrash(lua_State * L) {
	int t = lua_type(L, 1);
	switch (t) {
	case LUA_TSTRING: {
		break;
	}
	case LUA_TLIGHTUSERDATA: {
		void* msg = lua_touserdata(L, 1);
		luaL_checkinteger(L, 2);
		ls_free(msg);
		break;
	}
	default:
		luaL_error(L, "ls.trash invalid param %s", lua_typename(L, t));
	}

	return 0;
}

static int
lnow(lua_State * L) {
	uint64_t ti = ls_now();
	lua_pushinteger(L, ti);
	return 1;
}

static int
lhpc(lua_State * L) {
	lua_pushinteger(L, get_time());
	return 1;
}

#define MAX_LEVEL 3

struct source_info {
	const char* source;
	int line;
};

/*
	string tag
	string userstring
	thread co (default nil/current L)
	integer level (default nil)
 */
static int
ltrace(lua_State * L) {
	struct ls_context* context = (struct ls_context*)lua_touserdata(L, lua_upvalueindex(1));
	const char* tag = luaL_checkstring(L, 1);
	const char* user = luaL_checkstring(L, 2);
	if (!lua_isnoneornil(L, 3)) {
		lua_State* co = L;
		int level;
		if (lua_isthread(L, 3)) {
			co = lua_tothread(L, 3);
			level = luaL_optinteger(L, 4, 1);
		}
		else {
			level = luaL_optinteger(L, 3, 1);
		}
		struct source_info si[MAX_LEVEL];
		lua_Debug d;
		int index = 0;
		do {
			if (!lua_getstack(co, level, &d))
				break;
			lua_getinfo(co, "Sl", &d);
			level++;
			si[index].source = d.source;
			si[index].line = d.currentline;
			if (d.currentline >= 0)
				++index;
		} while (index < MAX_LEVEL);
		switch (index) {
		case 1:
			ls_error(context, "<TRACE %s> %lld %s : %s:%d", tag, get_time(), user, si[0].source, si[0].line);
			break;
		case 2:
			ls_error(context, "<TRACE %s> %lld %s : %s:%d %s:%d", tag, get_time(), user,
				si[0].source, si[0].line,
				si[1].source, si[1].line
			);
			break;
		case 3:
			ls_error(context, "<TRACE %s> %lld %s : %s:%d %s:%d %s:%d", tag, get_time(), user,
				si[0].source, si[0].line,
				si[1].source, si[1].line,
				si[2].source, si[2].line
			);
			break;
		default:
			ls_error(context, "<TRACE %s> %lld %s", tag, get_time(), user);
			break;
		}
		return 0;
	}
	ls_error(context, "<TRACE %s> %lld %s", tag, get_time(), user);
	return 0;
}

/*
用于 require 控制如何加载模块的表。
这张表内的每一项都是一个 查找器函数。
当查找一个模块时， require 按次序调用这些查找器，
并传入模块名（require 的参数）作为唯一的一个参数。
此函数可以返回另一个函数（模块的 加载器）加上另一个将传递给这个加载器的参数。
或是返回一个描述为何没有找到这个模块的字符串 （或是返回 nil 什么也不想说）。
Lua 用四个查找器函数初始化这张表。
第一个查找器就是简单的在 package.preload 表中查找加载器。
第二个查找器用于查找 Lua 库的加载库。它使用储存在 package.path 中的路径来做查找工作。
第三个查找器用于查找 C 库的加载库。 它使用储存在 package.cpath 中的路径来做查找工作。
例如，如果 C 路径是这样一个字符串 "./?.so;./?.dll;/usr/local/?/init.so"

这个 C 函数的名字是 "luaopen_" 紧接模块名的字符串， 其中字符串中所有的下划线都会被替换成点。
此外，如果模块名中有横线， 横线后面的部分（包括横线）都被去掉。
例如，如果模块名为 a.b.c-v2.1， 函数名就是 luaopen_a_b_c。
第四个搜索器是　一体化加载器。 它从 C 路径中查找指定模块的根名字。
例如，当请求 a.b.c　时， 它将查找 a 这个 C 库。 如果找得到，它会在里面找子模块的加载函数。
在我们的例子中，就是找　luaopen_a_b_c。
*/
//int luaopen_lovesnow_core(lua_State* L)
//{
//	luaL_Reg l[] = {
//		{"now", lnow},
//		{ NULL, NULL },
//	};
//	printf("in luaopen_lovesnow_core\n");
//	luaL_checkversion(L);
//	luaL_newlib(L, l);
//	return 1;// 把myLib表压入了栈中，所以就需要返回1  
//}
int luaopen_lovesnow_core(lua_State * L) {
	luaL_checkversion(L);

	luaL_Reg l[] = {
		{ "send" , lsend },
		{ "genid", lgenid },
		{ "redirect", lredirect },
		{ "command" , lcommand },
		{ "intcommand", lintcommand },
		{ "addresscommand", laddresscommand },
		{ "error", lerror },
		{ "harbor", lharbor },
		{ "callback", lcallback },
		{ "trace", ltrace },
		{ NULL, NULL },
	};

	// functions without ls_context
	luaL_Reg l2[] = {
		{ "tostring", ltostring },
		//TODO { "pack", luaseri_pack },
		//TODO { "unpack", luaseri_unpack },
		{ "packstring", lpackstring },
		{ "trash" , ltrash },
		{ "now", lnow },
		{ "hpc", lhpc },	// getHPCounter
		{ NULL, NULL },
	};

	lua_createtable(L, 0, sizeof(l) / sizeof(l[0]) + sizeof(l2) / sizeof(l2[0]) - 2);

	lua_getfield(L, LUA_REGISTRYINDEX, "ls_context");
	struct ls_context* ctx = (struct ls_context*)lua_touserdata(L, -1);
	if (ctx == NULL) {
		return luaL_error(L, "Init ls context first");
	}

	luaL_setfuncs(L, l, 1);

	luaL_setfuncs(L, l2, 0);

	return 1;
}