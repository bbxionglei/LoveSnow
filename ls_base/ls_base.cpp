// ls_base.cpp : 定义 DLL 的导出函数。
//

#include "framework.h"
#include "ls_base.h"

LSBASE_API int ls_base_func(void)
{
	printf("ls_base_func is call in ls_base\n");
    return 0;
}

LSBASE_API int ls_base_for_lua(void)
{
	printf("ls_base_for_lua is call in ls_base\n");
	return 999;
}
