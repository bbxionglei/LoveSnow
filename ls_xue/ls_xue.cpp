// ls_xue.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <ls_base/ls.h>

/*
Lua_lib			lua源码
ls_base			提供基本API
ls_gateway		模块:网关
ls_load_script	模块:加载脚本
ls_script_lib	提供脚本接口
ls_xue			主程序

Lua_base		封装lua接口
Lua_service		调用Lua_base封装成服务，被ls_load_script加载
examples		调用Lua_base完成业务逻辑，被Lua_service加载
*/

int main()
{
    std::cout << "Hello World!\n";
	auto module1 = ls_module_query("D:\\share_internal\\LoveSnow\\Bin\\?", "ls_gateway");
	auto module2 = ls_module_query("D:\\share_internal\\LoveSnow\\Bin\\?", "ls_load_script");
	module1->init(module1->create());
	module2->init(module2->create());
	//system("pause");
	return 0;
}
