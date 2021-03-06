﻿#include "framework.h"
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

	LSBASE_API void* ls_gateway_create(void);
	LSBASE_API void ls_gateway_release(void* g_);
	LSBASE_API int ls_gateway_init(void* g_, struct ls_context* ctx, const char* parm);
}