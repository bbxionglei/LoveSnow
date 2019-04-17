#ifndef LS_HARBOR_H
#define LS_HARBOR_H
#include "framework.h"
#include <stdint.h>
#include <stdlib.h>

#define GLOBALNAME_LENGTH 16
#define REMOTE_MAX 256

struct remote_name {
	char name[GLOBALNAME_LENGTH];
	uint32_t handle;
};

struct remote_message {
	struct remote_name destination;
	const void* message;
	size_t sz;
	int type;
};
extern "C" {
	LSBASE_API void ls_harbor_send(struct remote_message* rmsg, uint32_t source, int session);
	LSBASE_API int ls_harbor_message_isremote(uint32_t handle);
	LSBASE_API void ls_harbor_init(int harbor);
	LSBASE_API void ls_harbor_start(void* ctx);
	LSBASE_API void ls_harbor_exit();
}
#endif
