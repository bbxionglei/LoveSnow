#ifndef LOVESNOW_H
#define LOVESNOW_H

#include "framework.h"

#define THREAD_WORKER 0
#define THREAD_MAIN 1
#define THREAD_SOCKET 2
#define THREAD_TIMER 3
#define THREAD_MONITOR 4

extern "C" {

struct ls_config {
	int thread;
	int harbor;
	int profile;
	const char* daemon;
	const char* module_path;
	const char* bootstrap;
	const char* logger;
	const char* logservice;
};
LSBASE_API int ls_base_func(void);
LSBASE_API int ls_base_for_lua(void);
LSBASE_API void ls_start(struct ls_config* config);

}

#ifndef LS_H
#define LS_H

#include "ls_malloc.h"

#include <stddef.h>
#include <stdint.h>

#define PTYPE_TEXT 0
#define PTYPE_RESPONSE 1
#define PTYPE_MULTICAST 2
#define PTYPE_CLIENT 3
#define PTYPE_SYSTEM 4
#define PTYPE_HARBOR 5
#define PTYPE_SOCKET 6
// read lualib/ls.lua examples/simplemonitor.lua
#define PTYPE_ERROR 7	
// read lualib/ls.lua lualib/mqueue.lua lualib/snax.lua
#define PTYPE_RESERVED_QUEUE 8
#define PTYPE_RESERVED_DEBUG 9
#define PTYPE_RESERVED_LUA 10
#define PTYPE_RESERVED_SNAX 11

#define PTYPE_TAG_DONTCOPY 0x10000
#define PTYPE_TAG_ALLOCSESSION 0x20000
extern "C" {
	struct ls_context;
	LSBASE_API void ls_error(struct ls_context* context, const char* msg, ...);
	LSBASE_API const char* ls_command(struct ls_context* context, const char* cmd, const char* parm);
	LSBASE_API uint32_t ls_queryname(struct ls_context* context, const char* name);
	LSBASE_API int ls_send(struct ls_context* context, uint32_t source, uint32_t destination, int type, int session, void* msg, size_t sz);
	LSBASE_API int ls_sendname(struct ls_context* context, uint32_t source, const char* destination, int type, int session, void* msg, size_t sz);

	LSBASE_API int ls_isremote(struct ls_context*, uint32_t handle, int* harbor);

	LSBASE_API typedef int(*ls_cb)(struct ls_context* context, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz);
	LSBASE_API void ls_callback(struct ls_context* context, void* ud, ls_cb cb);

	LSBASE_API uint32_t ls_current_handle(void);
	LSBASE_API uint64_t ls_now(void);
	LSBASE_API void ls_debug_memory(const char* info);	// for debug use, output current service memory to stderr
}
#endif


#endif//LOVESNOW_H