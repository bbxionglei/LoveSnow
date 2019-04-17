#include "ls.h"
#include <lua.hpp>
#include <stdlib.h>
#include <assert.h>

struct ls_env {
	struct spinlock lock;
	lua_State* L;
};

static struct ls_env* E = NULL;

const char* ls_getenv(const char* key) {
	SPIN_LOCK(E);

	lua_State* L = E->L;

	lua_getglobal(L, key);
	const char* result = lua_tostring(L, -1);
	lua_pop(L, 1);

	SPIN_UNLOCK(E);

	return result;
}

void ls_setenv(const char* key, const char* value) {
	SPIN_LOCK(E);

	lua_State* L = E->L;
	lua_getglobal(L, key);
	assert(lua_isnil(L, -1));
	lua_pop(L, 1);
	lua_pushstring(L, value);
	lua_setglobal(L, key);

	SPIN_UNLOCK(E);
}

void ls_env_init() {
	E = (struct ls_env*)ls_malloc(sizeof(*E));
	SPIN_INIT(E);
	E->L = luaL_newstate();
}
