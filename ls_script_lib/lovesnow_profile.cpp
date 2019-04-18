#define LUA_LIB
#include "ls_script_lib.h"
#include <stdio.h>
#include <lua.hpp>

#include <time.h>
#include <chrono>
#include <ratio>

#if defined(__APPLE__)
#include <mach/task.h>
#include <mach/mach.h>
#endif

#define NANOSEC 1000000000
#define MICROSEC 1000000

// #define DEBUG_LOG

static double
get_time() {
	std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<uint64_t, std::nano>> now
		= std::chrono::time_point_cast<std::chrono::duration<uint64_t, std::nano>>(std::chrono::system_clock::now());
	uint64_t ns = now.time_since_epoch().count();
	int sec = (ns / NANOSEC) & 0xffff;
	int nsec = (ns % NANOSEC);
	return (double)sec + (double)nsec / NANOSEC;
}

static inline double
diff_time(double start) {
	double now = get_time();
	if (now < start) {
		return now + 0x10000 - start;
	}
	else {
		return now - start;
	}
}

static int
lstart(lua_State * L) {
	if (lua_gettop(L) != 0) {
		lua_settop(L, 1);
		luaL_checktype(L, 1, LUA_TTHREAD);
	}
	else {
		lua_pushthread(L);
	}
	lua_pushvalue(L, 1);	// push coroutine
	lua_rawget(L, lua_upvalueindex(2));
	if (!lua_isnil(L, -1)) {
		return luaL_error(L, "Thread %p start profile more than once", lua_topointer(L, 1));
	}
	lua_pushvalue(L, 1);	// push coroutine
	lua_pushnumber(L, 0);
	lua_rawset(L, lua_upvalueindex(2));

	lua_pushvalue(L, 1);	// push coroutine
	double ti = get_time();
#ifdef DEBUG_LOG
	fprintf(stderr, "PROFILE [%p] start\n", L);
#endif
	lua_pushnumber(L, ti);
	lua_rawset(L, lua_upvalueindex(1));

	return 0;
}

static int
lstop(lua_State * L) {
	if (lua_gettop(L) != 0) {
		lua_settop(L, 1);
		luaL_checktype(L, 1, LUA_TTHREAD);
	}
	else {
		lua_pushthread(L);
	}
	lua_pushvalue(L, 1);	// push coroutine
	lua_rawget(L, lua_upvalueindex(1));
	if (lua_type(L, -1) != LUA_TNUMBER) {
		return luaL_error(L, "Call profile.start() before profile.stop()");
	}
	double ti = diff_time(lua_tonumber(L, -1));
	lua_pushvalue(L, 1);	// push coroutine
	lua_rawget(L, lua_upvalueindex(2));
	double total_time = lua_tonumber(L, -1);

	lua_pushvalue(L, 1);	// push coroutine
	lua_pushnil(L);
	lua_rawset(L, lua_upvalueindex(1));

	lua_pushvalue(L, 1);	// push coroutine
	lua_pushnil(L);
	lua_rawset(L, lua_upvalueindex(2));

	total_time += ti;
	lua_pushnumber(L, total_time);
#ifdef DEBUG_LOG
	fprintf(stderr, "PROFILE [%p] stop (%lf/%lf)\n", lua_tothread(L, 1), ti, total_time);
#endif

	return 1;
}

static int
timing_resume(lua_State * L) {
	lua_pushvalue(L, -1);
	lua_rawget(L, lua_upvalueindex(2));
	if (lua_isnil(L, -1)) {		// check total time
		lua_pop(L, 2);	// pop from coroutine
	}
	else {
		lua_pop(L, 1);
		double ti = get_time();
#ifdef DEBUG_LOG
		fprintf(stderr, "PROFILE [%p] resume %lf\n", lua_tothread(L, -1), ti);
#endif
		lua_pushnumber(L, ti);
		lua_rawset(L, lua_upvalueindex(1));	// set start time
	}

	lua_CFunction co_resume = lua_tocfunction(L, lua_upvalueindex(3));

	return co_resume(L);
}

static int
lresume(lua_State * L) {
	lua_pushvalue(L, 1);

	return timing_resume(L);
}

static int
lresume_co(lua_State * L) {
	luaL_checktype(L, 2, LUA_TTHREAD);
	lua_rotate(L, 2, -1);	// 'from' coroutine rotate to the top(index -1)

	return timing_resume(L);
}

static int
timing_yield(lua_State * L) {
#ifdef DEBUG_LOG
	lua_State* from = lua_tothread(L, -1);
#endif
	lua_pushvalue(L, -1);
	lua_rawget(L, lua_upvalueindex(2));	// check total time
	if (lua_isnil(L, -1)) {
		lua_pop(L, 2);
	}
	else {
		double ti = lua_tonumber(L, -1);
		lua_pop(L, 1);

		lua_pushvalue(L, -1);	// push coroutine
		lua_rawget(L, lua_upvalueindex(1));
		double starttime = lua_tonumber(L, -1);
		lua_pop(L, 1);

		double diff = diff_time(starttime);
		ti += diff;
#ifdef DEBUG_LOG
		fprintf(stderr, "PROFILE [%p] yield (%lf/%lf)\n", from, diff, ti);
#endif

		lua_pushvalue(L, -1);	// push coroutine
		lua_pushnumber(L, ti);
		lua_rawset(L, lua_upvalueindex(2));
		lua_pop(L, 1);	// pop coroutine
	}

	lua_CFunction co_yield_ = lua_tocfunction(L, lua_upvalueindex(3));

	return co_yield_(L);
}

static int
lyield(lua_State * L) {
	lua_pushthread(L);

	return timing_yield(L);
}

static int
lyield_co(lua_State * L) {
	luaL_checktype(L, 1, LUA_TTHREAD);
	lua_rotate(L, 1, -1);

	return timing_yield(L);
}

int luaopen_lovesnow_profile(lua_State * L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "start", lstart },
		{ "stop", lstop },
		{ "resume", lresume },
		{ "yield", lyield },
		{ "resume_co", lresume_co },
		{ "yield_co", lyield_co },
		{ NULL, NULL },
	};
	luaL_newlibtable(L, l);
	lua_newtable(L);	// table thread->start time
	lua_newtable(L);	// table thread->total time

	lua_newtable(L);	// weak table
	lua_pushliteral(L, "kv");
	lua_setfield(L, -2, "__mode");

	lua_pushvalue(L, -1);
	lua_setmetatable(L, -3);
	lua_setmetatable(L, -3);

	lua_pushnil(L);	// cfunction (coroutine.resume or coroutine.yield)
	luaL_setfuncs(L, l, 3);

	int libtable = lua_gettop(L);

	lua_getglobal(L, "coroutine");
	lua_getfield(L, -1, "resume");

	lua_CFunction co_resume = lua_tocfunction(L, -1);
	if (co_resume == NULL)
		return luaL_error(L, "Can't get coroutine.resume");
	lua_pop(L, 1);

	lua_getfield(L, libtable, "resume");
	lua_pushcfunction(L, co_resume);
	lua_setupvalue(L, -2, 3);
	lua_pop(L, 1);

	lua_getfield(L, libtable, "resume_co");
	lua_pushcfunction(L, co_resume);
	lua_setupvalue(L, -2, 3);
	lua_pop(L, 1);

	lua_getfield(L, -1, "yield");

	lua_CFunction co_yield_ = lua_tocfunction(L, -1);
	if (co_yield_ == NULL)
		return luaL_error(L, "Can't get coroutine.yield");
	lua_pop(L, 1);

	lua_getfield(L, libtable, "yield");
	lua_pushcfunction(L, co_yield_);
	lua_setupvalue(L, -2, 3);
	lua_pop(L, 1);

	lua_getfield(L, libtable, "yield_co");
	lua_pushcfunction(L, co_yield_);
	lua_setupvalue(L, -2, 3);
	lua_pop(L, 1);

	lua_settop(L, libtable);

	return 1;
}
