#include "framework.h"
#include <lua.hpp>

extern "C" {

	LSBASE_API int luaopen_lovesnow_ff1(lua_State* L);
	LSBASE_API int luaopen_lovesnow_ff2(lua_State* L);
	LSBASE_API int luaopen_lovesnow_core(lua_State* L);
	LSBASE_API int luaopen_lovesnow_profile(lua_State* L);
	LSBASE_API int luaopen_lovesnow_cluster_core(lua_State* L);
	LSBASE_API int luaopen_lovesnow_datasheet_core(lua_State* L);
	LSBASE_API int luaopen_lovesnow_debugchannel(lua_State* L);
	LSBASE_API int luaopen_lovesnow_memory(lua_State* L);
	LSBASE_API int luaopen_lovesnow_mongo_driver(lua_State* L);
	LSBASE_API int luaopen_lovesnow_multicast_core(lua_State* L);
	LSBASE_API int luaopen_lovesnow_netpack(lua_State* L);
	LSBASE_API int luaopen_lovesnow_sharedata_core(lua_State* L);
	LSBASE_API int luaopen_lovesnow_socketdriver(lua_State* L);
	LSBASE_API int luaopen_lovesnow_stm(lua_State* L);
	LSBASE_API int luaopen_lovesnow_crypt(lua_State* L);

}