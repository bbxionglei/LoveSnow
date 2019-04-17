#include "ls.h"

#include "ls_socket.h"
#include "socket_server.h"
#include "ls_server.h"
#include "ls_mq.h"
#include "ls_harbor.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static struct socket_server* SOCKET_SERVER = NULL;

void
ls_socket_init() {
	SOCKET_SERVER = socket_server_create(ls_now());
}

void
ls_socket_exit() {
	socket_server_exit(SOCKET_SERVER);
}

void
ls_socket_free() {
	socket_server_release(SOCKET_SERVER);
	SOCKET_SERVER = NULL;
}

void
ls_socket_updatetime() {
	socket_server_updatetime(SOCKET_SERVER, ls_now());
}

// mainloop thread
static void
forward_message(int type, bool padding, struct socket_message* result) {
	struct ls_socket_message* sm;
	size_t sz = sizeof(*sm);
	if (padding) {
		if (result->data) {
			size_t msg_sz = strlen(result->data);
			if (msg_sz > 128) {
				msg_sz = 128;
			}
			sz += msg_sz;
		}
		else {
			result->data = (char*)"";
		}
	}
	sm = (struct ls_socket_message*)ls_malloc(sz);
	sm->type = type;
	sm->id = result->id;
	sm->ud = result->ud;
	if (padding) {
		sm->buffer = NULL;
		memcpy(sm + 1, result->data, sz - sizeof(*sm));
	}
	else {
		sm->buffer = result->data;
	}

	struct ls_message message;
	message.source = 0;
	message.session = 0;
	message.data = sm;
	message.sz = sz | ((size_t)PTYPE_SOCKET << MESSAGE_TYPE_SHIFT);

	if (ls_context_push((uint32_t)result->opaque, &message)) {
		// todo: report somewhere to close socket
		// don't call ls_socket_close here (It will block mainloop)
		ls_free(sm->buffer);
		ls_free(sm);
	}
}

int
ls_socket_poll() {
	struct socket_server* ss = SOCKET_SERVER;
	assert(ss);
	struct socket_message result;
	int more = 1;
	int type = socket_server_poll(ss, &result, &more);
	switch (type) {
	case SOCKET_EXIT:
		return 0;
	case SOCKET_DATA:
		forward_message(LS_SOCKET_TYPE_DATA, false, &result);
		break;
	case SOCKET_CLOSE:
		forward_message(LS_SOCKET_TYPE_CLOSE, false, &result);
		break;
	case SOCKET_OPEN:
		forward_message(LS_SOCKET_TYPE_CONNECT, true, &result);
		break;
	case SOCKET_ERR:
		forward_message(LS_SOCKET_TYPE_ERROR, true, &result);
		break;
	case SOCKET_ACCEPT:
		forward_message(LS_SOCKET_TYPE_ACCEPT, true, &result);
		break;
	case SOCKET_UDP:
		forward_message(LS_SOCKET_TYPE_UDP, false, &result);
		break;
	case SOCKET_WARNING:
		forward_message(LS_SOCKET_TYPE_WARNING, false, &result);
		break;
	default:
		ls_error(NULL, "Unknown socket message type %d.", type);
		return -1;
	}
	if (more) {
		return -1;
	}
	return 1;
}

int
ls_socket_send(struct ls_context* ctx, int id, void* buffer, int sz) {
	return socket_server_send(SOCKET_SERVER, id, buffer, sz);
}

int
ls_socket_send_lowpriority(struct ls_context* ctx, int id, void* buffer, int sz) {
	return socket_server_send_lowpriority(SOCKET_SERVER, id, buffer, sz);
}

int
ls_socket_listen(struct ls_context* ctx, const char* host, int port, int backlog) {
	uint32_t source = ls_context_handle(ctx);
	return socket_server_listen(SOCKET_SERVER, source, host, port, backlog);
}

int
ls_socket_connect(struct ls_context* ctx, const char* host, int port) {
	uint32_t source = ls_context_handle(ctx);
	return socket_server_connect(SOCKET_SERVER, source, host, port);
}

int
ls_socket_bind(struct ls_context* ctx, int fd) {
	uint32_t source = ls_context_handle(ctx);
	return socket_server_bind(SOCKET_SERVER, source, fd);
}

void
ls_socket_close(struct ls_context* ctx, int id) {
	uint32_t source = ls_context_handle(ctx);
	socket_server_close(SOCKET_SERVER, source, id);
}

void
ls_socket_shutdown(struct ls_context* ctx, int id) {
	uint32_t source = ls_context_handle(ctx);
	socket_server_shutdown(SOCKET_SERVER, source, id);
}

void
ls_socket_start(struct ls_context* ctx, int id) {
	uint32_t source = ls_context_handle(ctx);
	socket_server_start(SOCKET_SERVER, source, id);
}

void
ls_socket_nodelay(struct ls_context* ctx, int id) {
	socket_server_nodelay(SOCKET_SERVER, id);
}

int
ls_socket_udp(struct ls_context* ctx, const char* addr, int port) {
	uint32_t source = ls_context_handle(ctx);
	return socket_server_udp(SOCKET_SERVER, source, addr, port);
}

int
ls_socket_udp_connect(struct ls_context* ctx, int id, const char* addr, int port) {
	return socket_server_udp_connect(SOCKET_SERVER, id, addr, port);
}

int
ls_socket_udp_send(struct ls_context* ctx, int id, const char* address, const void* buffer, int sz) {
	return socket_server_udp_send(SOCKET_SERVER, id, (const struct socket_udp_address*)address, buffer, sz);
}

const char*
ls_socket_udp_address(struct ls_socket_message* msg, int* addrsz) {
	if (msg->type != LS_SOCKET_TYPE_UDP) {
		return NULL;
	}
	struct socket_message sm;
	sm.id = msg->id;
	sm.opaque = 0;
	sm.ud = msg->ud;
	sm.data = msg->buffer;
	return (const char*)socket_server_udp_address(SOCKET_SERVER, &sm, addrsz);
}

struct socket_info*
	ls_socket_info() {
	return socket_server_info(SOCKET_SERVER);
}
