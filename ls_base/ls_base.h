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

LSBASE_API int ls_base_func(void);
LSBASE_API int ls_base_for_lua(void);

}