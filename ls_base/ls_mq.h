#ifndef LS_MESSAGE_QUEUE_H
#define LS_MESSAGE_QUEUE_H

#include "framework.h"
#include <stdlib.h>
#include <stdint.h>

struct ls_message {
	uint32_t source;
	int session;
	void* data;
	size_t sz;
};

// type is encoding in ls_message.sz high 8bit
#define MESSAGE_TYPE_MASK (SIZE_MAX >> 8)
#define MESSAGE_TYPE_SHIFT ((sizeof(size_t)-1) * 8)

struct message_queue;
extern "C" {
	LSBASE_API void ls_globalmq_push(struct message_queue* queue);
	LSBASE_API struct message_queue* ls_globalmq_pop(void);

	LSBASE_API struct message_queue* ls_mq_create(uint32_t handle);
	LSBASE_API void ls_mq_mark_release(struct message_queue* q);

	LSBASE_API typedef void(*message_drop)(struct ls_message*, void*);

	LSBASE_API void ls_mq_release(struct message_queue* q, message_drop drop_func, void* ud);
	LSBASE_API uint32_t ls_mq_handle(struct message_queue*);

	// 0 for success
	LSBASE_API int ls_mq_pop(struct message_queue* q, struct ls_message* message);
	LSBASE_API void ls_mq_push(struct message_queue* q, struct ls_message* message);

	// return the length of message queue, for debug
	LSBASE_API int ls_mq_length(struct message_queue* q);
	LSBASE_API int ls_mq_overload(struct message_queue* q);

	LSBASE_API void ls_mq_init();
}
#endif
