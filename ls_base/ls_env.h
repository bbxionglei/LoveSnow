#ifndef LS_ENV_H
#define LS_ENV_H
#include "framework.h"
extern "C" {
	LSBASE_API const char* ls_getenv(const char* key);
	LSBASE_API void ls_setenv(const char* key, const char* value);
	LSBASE_API void ls_env_init();
}
#endif
