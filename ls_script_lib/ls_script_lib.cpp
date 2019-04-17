// ls_base.cpp : 定义 DLL 的导出函数。
//

#include "framework.h"
#include "ls_script_lib.h"
#include <lua.hpp>
#include <ls_base/ls.h>

LSBASE_API int ls_script_lib_init(void)
{
	printf("ls_script_lib_init is call in ls_script_lib\n");
    return 0;
}

static int dllHello(lua_State* L)
{
	printf("in dllHello ls_base_for_lua=%d\n", ls_base_for_lua());
	return 0;
}
static int dllFF1(lua_State* L)
{
	printf("in dllFF1\n");
	return 0;
}
static int dllFF2(lua_State* L)
{
	printf("in dllFF2\n");
	return 0;
}

int luaopen_lovesnow(lua_State* L)
{
	luaL_Reg l[] = {
		{"dllHello", dllHello},
		{ NULL, NULL },
	};
	printf("in luaopen_lovesnow\n");
	luaL_checkversion(L);
	luaL_newlib(L, l);
	return 1;// 把myLib表压入了栈中，所以就需要返回1  
}
int luaopen_lovesnow_ff1(lua_State* L)
{
	luaL_Reg l[] = {
		{"subff", dllFF1},
		{ NULL, NULL },
	};
	printf("in luaopen_lovesnow_ff1\n");
	luaL_checkversion(L);
	luaL_newlib(L, l);
	return 1;// 把myLib表压入了栈中，所以就需要返回1  
}
int luaopen_lovesnow_ff2(lua_State* L)
{
	luaL_Reg l[] = {
		{"subff", dllFF2},
		{ NULL, NULL },
	};
	printf("in luaopen_lovesnow_ff2\n");
	luaL_checkversion(L);
	luaL_newlib(L, l);
	return 1;// 把myLib表压入了栈中，所以就需要返回1  
}
