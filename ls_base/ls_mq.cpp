#include "ls.h"
#include "ls_handle.h"
#include "spinlock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define DEFAULT_QUEUE_SIZE 64
#define MAX_GLOBAL_MQ 0x10000

// 0 means mq is not in global mq.
// 1 means mq is in global mq , or the message is dispatching.

#define MQ_IN_GLOBAL 1
#define MQ_OVERLOAD 1024

struct message_queue {
	struct spinlock lock;
	uint32_t handle;
	int cap;
	int head;
	int tail;
	int release;
	int in_global;
	int overload;
	int overload_threshold;
	struct ls_message* queue;
	struct message_queue* next;
};

struct global_queue {
	struct message_queue* head;
	struct message_queue* tail;
	struct spinlock lock;
};

static struct global_queue* Q = NULL;

void
ls_globalmq_push(struct message_queue* queue) {
	struct global_queue* q = Q;

	SPIN_LOCK(q)
		assert(queue->next == NULL);
	if (q->tail) {
		q->tail->next = queue;
		q->tail = queue;
	}
	else {
		q->head = q->tail = queue;
	}
	SPIN_UNLOCK(q)
}

struct message_queue*
	ls_globalmq_pop() {
	struct global_queue* q = Q;

	SPIN_LOCK(q)
		struct message_queue* mq = q->head;
	if (mq) {
		q->head = mq->next;
		if (q->head == NULL) {
			assert(mq == q->tail);
			q->tail = NULL;
		}
		mq->next = NULL;
	}
	SPIN_UNLOCK(q)

		return mq;
}

struct message_queue*
	ls_mq_create(uint32_t handle) {
	struct message_queue* q = (struct message_queue*)ls_malloc(sizeof(*q));
	q->handle = handle;
	q->cap = DEFAULT_QUEUE_SIZE;
	q->head = 0;
	q->tail = 0;
	SPIN_INIT(q)
		// When the queue is create (always between service create and service init) ,
		// set in_global flag to avoid push it to global queue .
		// If the service init success, ls_context_new will call ls_mq_push to push it to global queue.
		q->in_global = MQ_IN_GLOBAL;
	q->release = 0;
	q->overload = 0;
	q->overload_threshold = MQ_OVERLOAD;
	q->queue = (struct ls_message*)ls_malloc(sizeof(struct ls_message) * q->cap);
	q->next = NULL;

	return q;
}

static void
_release(struct message_queue* q) {
	assert(q->next == NULL);
	SPIN_DESTROY(q)
		ls_free(q->queue);
	ls_free(q);
}

uint32_t
ls_mq_handle(struct message_queue* q) {
	return q->handle;
}

int
ls_mq_length(struct message_queue* q) {
	int head, tail, cap;

	SPIN_LOCK(q)
		head = q->head;
	tail = q->tail;
	cap = q->cap;
	SPIN_UNLOCK(q)

		if (head <= tail) {
			return tail - head;
		}
	return tail + cap - head;
}

int
ls_mq_overload(struct message_queue* q) {
	if (q->overload) {
		int overload = q->overload;
		q->overload = 0;
		return overload;
	}
	return 0;
}

int
ls_mq_pop(struct message_queue* q, struct ls_message* message) {
	int ret = 1;
	SPIN_LOCK(q)

		if (q->head != q->tail) {
			*message = q->queue[q->head++];
			ret = 0;
			int head = q->head;
			int tail = q->tail;
			int cap = q->cap;

			if (head >= cap) {
				q->head = head = 0;
			}
			int length = tail - head;
			if (length < 0) {
				length += cap;
			}
			while (length > q->overload_threshold) {
				q->overload = length;
				q->overload_threshold *= 2;
			}
		}
		else {
			// reset overload_threshold when queue is empty
			q->overload_threshold = MQ_OVERLOAD;
		}

	if (ret) {
		q->in_global = 0;
	}

	SPIN_UNLOCK(q)

		return ret;
}

static void
expand_queue(struct message_queue* q) {
	struct ls_message* new_queue = (struct ls_message*)ls_malloc(sizeof(struct ls_message) * q->cap * 2);
	int i;
	for (i = 0; i < q->cap; i++) {
		new_queue[i] = q->queue[(q->head + i) % q->cap];
	}
	q->head = 0;
	q->tail = q->cap;
	q->cap *= 2;

	ls_free(q->queue);
	q->queue = new_queue;
}

void
ls_mq_push(struct message_queue* q, struct ls_message* message) {
	assert(message);
	SPIN_LOCK(q)

		q->queue[q->tail] = *message;
	if (++q->tail >= q->cap) {
		q->tail = 0;
	}

	if (q->head == q->tail) {
		expand_queue(q);
	}

	if (q->in_global == 0) {
		q->in_global = MQ_IN_GLOBAL;
		ls_globalmq_push(q);
	}

	SPIN_UNLOCK(q)
}

void
ls_mq_init() {
	struct global_queue* q = (struct global_queue*)ls_malloc(sizeof(*q));
	memset(q, 0, sizeof(*q));
	SPIN_INIT(q);
	Q = q;
}

void
ls_mq_mark_release(struct message_queue* q) {
	SPIN_LOCK(q)
		assert(q->release == 0);
	q->release = 1;
	if (q->in_global != MQ_IN_GLOBAL) {
		ls_globalmq_push(q);
	}
	SPIN_UNLOCK(q)
}

static void
_drop_queue(struct message_queue* q, message_drop drop_func, void* ud) {
	struct ls_message msg;
	while (!ls_mq_pop(q, &msg)) {
		drop_func(&msg, ud);
	}
	_release(q);
}

void
ls_mq_release(struct message_queue* q, message_drop drop_func, void* ud) {
	SPIN_LOCK(q)

		if (q->release) {
			SPIN_UNLOCK(q)
				_drop_queue(q, drop_func, ud);
		}
		else {
			ls_globalmq_push(q);
			SPIN_UNLOCK(q)
		}
}
