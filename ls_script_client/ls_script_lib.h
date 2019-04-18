#include "framework.h"
#include <lua.hpp>

extern "C" {

	LSBASE_API int luaopen_client_socket(lua_State* L);
	LSBASE_API int luaopen_client_crypt(lua_State* L);

}