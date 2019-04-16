#include "framework.h"
#ifdef LSBASE_EXPORTS
#define LSBASE_API __declspec(dllexport)
#else
#define LSBASE_API __declspec(dllimport)
#endif
EXTERN_C_START

LSBASE_API void* ls_gateway_create(void);
LSBASE_API int ls_gateway_init(void* inst);

EXTERN_C_END