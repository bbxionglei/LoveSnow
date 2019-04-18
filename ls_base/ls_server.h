#ifndef LS_SERVER_H
#define LS_SERVER_H
#include "framework.h"

#include <stdint.h>
#include <stdlib.h>

struct ls_context;
struct ls_message;
struct ls_monitor;

extern "C" {
	LSBASE_API struct ls_context* ls_context_new(const char* name, const char* parm);//创建服务，注册服务，初始化消息队列服务之前根据服务的handle通信
	LSBASE_API void ls_context_grab(struct ls_context*);
	LSBASE_API void ls_context_reserve(struct ls_context* ctx);
	LSBASE_API struct ls_context* ls_context_release(struct ls_context*);
	LSBASE_API uint32_t ls_context_handle(struct ls_context*);
	LSBASE_API int ls_context_push(uint32_t handle, struct ls_message* message);
	LSBASE_API void ls_context_send(struct ls_context* context, void* msg, size_t sz, uint32_t source, int type, int session);
	LSBASE_API int ls_context_newsession(struct ls_context*);
	LSBASE_API struct message_queue* ls_context_message_dispatch(struct ls_monitor*, struct message_queue*, int weight);	// return next queue
	LSBASE_API int ls_context_total();
	LSBASE_API void ls_context_dispatchall(struct ls_context* context);	// for ls_error output before exit

	LSBASE_API void ls_context_endless(uint32_t handle);	// for monitor

	LSBASE_API void ls_globalinit(void);
	LSBASE_API void ls_globalexit(void);
	LSBASE_API void ls_initthread(int m);

	LSBASE_API void ls_profile_enable(int enable);
	LSBASE_API char* strsep_(char** stringp, const char* delim);
}
#endif
