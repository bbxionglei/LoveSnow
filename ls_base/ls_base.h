#include "framework.h"

#ifdef LSBASE_EXPORTS
#define LSBASE_API __declspec(dllexport)
#else
#define LSBASE_API __declspec(dllimport)
#endif
EXTERN_C_START

LSBASE_API int ls_base_func(void);
LSBASE_API int ls_base_for_lua(void);

EXTERN_C_END