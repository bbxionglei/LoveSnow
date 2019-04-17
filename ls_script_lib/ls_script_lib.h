#include "framework.h"
#include <lua.hpp>

extern "C" {

	LSBASE_API int ls_script_lib_init(void);
	LSBASE_API int luaopen_lovesnow(lua_State* L);
	LSBASE_API int luaopen_lovesnow_ff1(lua_State* L);
	LSBASE_API int luaopen_lovesnow_ff2(lua_State* L);
	LSBASE_API int luaopen_lovesnow_core(lua_State* L);
	LSBASE_API int luaopen_lovesnow_profile(lua_State* L);

}