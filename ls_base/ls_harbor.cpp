#include "ls.h"
#include "ls_harbor.h"
#include "ls_server.h"
#include "ls_mq.h"
#include "ls_handle.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

static struct ls_context* REMOTE = 0;
static unsigned int HARBOR = ~0;

static inline int
invalid_type(int type) {
	return type != PTYPE_SYSTEM && type != PTYPE_HARBOR;
}

void
ls_harbor_send(struct remote_message* rmsg, uint32_t source, int session) {
	assert(invalid_type(rmsg->type) && REMOTE);
	ls_context_send(REMOTE, rmsg, sizeof(*rmsg), source, PTYPE_SYSTEM, session);
}

int
ls_harbor_message_isremote(uint32_t handle) {
	assert(HARBOR != ~0);
	int h = (handle & ~HANDLE_MASK);
	return h != HARBOR && h != 0;
}

void
ls_harbor_init(int harbor) {
	HARBOR = (unsigned int)harbor << HANDLE_REMOTE_SHIFT;
}

void
ls_harbor_start(void* ctx) {
	// the HARBOR must be reserved to ensure the pointer is valid.
	// It will be released at last by calling ls_harbor_exit
	ls_context_reserve((struct ls_context*)ctx);
	REMOTE = (struct ls_context*)ctx;
}

void
ls_harbor_exit() {
	struct ls_context* ctx = REMOTE;
	REMOTE = NULL;
	if (ctx) {
		ls_context_release(ctx);
	}
}
