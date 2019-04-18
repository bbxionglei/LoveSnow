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
LSBASE_API void* ls_load_script_create(void);
LSBASE_API int ls_load_script_init(void* l_, struct ls_context* ctx, const char* args);
LSBASE_API void ls_load_script_release(void* l_);
LSBASE_API void ls_load_script_signal(void* l_, int signal);
}