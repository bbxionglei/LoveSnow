#ifndef LS_TIMER_H
#define LS_TIMER_H
#include "framework.h"

#include <stdint.h>
extern "C" {
	LSBASE_API int ls_timeout(uint32_t handle, int time, int session);
	LSBASE_API void ls_updatetime(void);
	LSBASE_API uint32_t ls_starttime(void);
	LSBASE_API uint64_t ls_thread_time(void);	// for profile, in micro second

	LSBASE_API void ls_timer_init(void);
}
#endif
