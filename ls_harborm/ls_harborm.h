// ls_base.cpp : 定义 DLL 的导出函数。
//
#include "framework.h"

#ifdef _WIN32
#ifdef LSBASE_EXPORTS
#define LSBASE_API __declspec(dllexport)
#else
#define LSBASE_API __declspec(dllimport)
#endif
#else
#define LSBASE_API 
#endif

extern "C" {
	LSBASE_API void* ls_harborm_create(void);
	LSBASE_API void ls_harborm_release(void* inst_);
	LSBASE_API int ls_harborm_init(void* inst_, struct ls_context* ctx, const char* parm);
}
