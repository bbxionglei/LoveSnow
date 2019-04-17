#ifndef LS_MALLOC_HOOK_H
#define LS_MALLOC_HOOK_H
#include "framework.h"
#include <stdlib.h>
#include <lua.hpp>
extern "C" {
	LSBASE_API size_t malloc_used_memory(void);
	LSBASE_API size_t malloc_memory_block(void);
	LSBASE_API void   memory_info_dump(void);
	LSBASE_API size_t mallctl_int64(const char* name, size_t* newval);
	LSBASE_API int    mallctl_opt(const char* name, int* newval);
	LSBASE_API void   dump_c_mem(void);
	LSBASE_API int    dump_mem_lua(lua_State* L);
	LSBASE_API size_t malloc_current_memory(void);
}
#endif /* LS_MALLOC_HOOK_H */

