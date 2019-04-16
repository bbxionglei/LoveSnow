// ls_base.cpp : 定义 DLL 的导出函数。
//

#include "framework.h"
#include "ls_load_script.h"
#include <ls_base/ls.h>

#include <lua.hpp>


static int exeHello(lua_State* L)
{
	printf("in exeHello\n");
	return 0;
}
LSBASE_API void* ls_load_script_create(void)
{
	printf("ls_load_script_create is call in ls_load_script\n");
    return nullptr;
}
int luaopenGetLib(lua_State* L)
{
	luaL_Reg l[] = {
	{"exeHello", exeHello},
	{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1; //return one value
}
LSBASE_API int ls_load_script_init(void* inst)
{
	printf("ls_load_script_init   is call in ls_load_script\n");
	ls_base_func();
	do
	{

		//1.创建Lua状态  
		lua_State* L = luaL_newstate();
		if (L == NULL)
		{
			printf("ls_load_script_init  luaL_newstate error\n");
			break;
		}
		luaL_openlibs(L); //打开以上所有的lib
		luaL_requiref(L, "ls", luaopenGetLib, 1);
		//2.加载Lua文件  
		int bRet = luaL_loadfile(L, "Lua_base.lua");
		if (bRet)
		{
			printf("ls_load_script_init  luaL_loadfile error\n");
			break;
		}

		//3.运行Lua文件  
		bRet = lua_pcall(L, 0, 0, 0);
		if (bRet)
		{
			printf("ls_load_script_init  lua_pcall error\n");
			break;
		}
	} while (0);

	return 0;
}
