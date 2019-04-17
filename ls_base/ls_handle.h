#ifndef LS_CONTEXT_HANDLE_H
#define LS_CONTEXT_HANDLE_H
#include "framework.h"
#include <stdint.h>

// reserve high 8 bits for remote id
#define HANDLE_MASK 0xffffff
#define HANDLE_REMOTE_SHIFT 24

struct ls_context;
extern "C" {
	LSBASE_API uint32_t ls_handle_register(struct ls_context*);
	LSBASE_API int ls_handle_retire(uint32_t handle);
	LSBASE_API struct ls_context* ls_handle_grab(uint32_t handle);
	LSBASE_API void ls_handle_retireall();

	LSBASE_API uint32_t ls_handle_findname(const char* name);
	LSBASE_API const char* ls_handle_namehandle(uint32_t handle, const char* name);

	LSBASE_API void ls_handle_init(int harbor);
}
#endif
