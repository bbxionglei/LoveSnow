// ls_base.cpp : 定义 DLL 的导出函数。
//

#include "framework.h"
#include "ls_script_lib.h"
#include <lua.hpp>
#include <ls_base/ls.h>

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
