// ls_base.cpp : 定义 DLL 的导出函数。
//

#include "framework.h"
#include "ls_gateway.h"
#include <ls_base/ls.h>

LSBASE_API void* ls_gateway_create(void)
{
	return NULL;
}

LSBASE_API int ls_gateway_init(void* inst)
{
	ls_base_func();
	return 0;
}
