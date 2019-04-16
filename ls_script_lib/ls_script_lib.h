#include "framework.h"
#include <lua.hpp>

#ifdef LSBASE_EXPORTS
#define LSBASE_API __declspec(dllexport)
#else
#define LSBASE_API __declspec(dllimport)
#endif
EXTERN_C_START

LSBASE_API int ls_script_lib_init(void);
LSBASE_API int luaopen_mLualib(lua_State* L);

EXTERN_C_END