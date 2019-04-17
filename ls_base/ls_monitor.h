#ifndef LS_MONITOR_H
#define LS_MONITOR_H
#include "framework.h"

#include <stdint.h>

struct ls_monitor;
extern "C" {
	LSBASE_API struct ls_monitor* ls_monitor_new();
	LSBASE_API void ls_monitor_delete(struct ls_monitor*);
	LSBASE_API void ls_monitor_trigger(struct ls_monitor*, uint32_t source, uint32_t destination);
	LSBASE_API void ls_monitor_check(struct ls_monitor*);
}
#endif
