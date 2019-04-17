#ifndef ls_log_h
#define ls_log_h

#include "framework.h"
#include "ls_env.h"
#include "ls.h"

#include <stdio.h>
#include <stdint.h>
extern "C" {
	LSBASE_API FILE* ls_log_open(struct ls_context* ctx, uint32_t handle);
	LSBASE_API void ls_log_close(struct ls_context* ctx, FILE* f, uint32_t handle);
	LSBASE_API void ls_log_output(FILE* f, uint32_t source, int type, int session, void* buffer, size_t sz);
}
#endif