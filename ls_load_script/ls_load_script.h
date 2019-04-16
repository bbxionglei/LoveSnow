#include "framework.h"
#ifdef LSBASE_EXPORTS
#define LSBASE_API __declspec(dllexport)
#else
#define LSBASE_API __declspec(dllimport)
#endif
EXTERN_C_START

LSBASE_API void* ls_load_script_create(void);
LSBASE_API int ls_load_script_init(void* inst);

EXTERN_C_END