#include "framework.h"
#include <lua.hpp>

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

LSBASE_API int ls_script_lib_init(void);
LSBASE_API int luaopen_mLualib(lua_State* L);

}