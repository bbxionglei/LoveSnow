#include "ls_gateway.h"
#include <ls_base/ls.h>
#include "ls_base/ls_socket.h"
#include "databuffer.h"
#include "hashid.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#define BACKLOG 128

struct connection {
	int id;	// ls_socket id
	uint32_t agent;
	uint32_t client;
	char remote_name[32];
	struct databuffer buffer;
};

struct gate {
	struct ls_context* ctx;
	int listen_id;
	uint32_t watchdog;
	uint32_t broker;
	int client_tag;
	int header_size;
	int max_connection;
	struct hashid hash;
	struct connection* conn;
	// todo: save message pool ptr for release
	struct messagepool mp;
};

void* ls_gateway_create(void) {
	struct gate* g = (struct gate*)ls_malloc(sizeof(*g));
	memset(g, 0, sizeof(*g));
	g->listen_id = -1;
	return g;
}

void ls_gateway_release(void* g_) {
	struct gate* g = (struct gate*)g_;
	int i;
	struct ls_context* ctx = g->ctx;
	for (i = 0; i < g->max_connection; i++) {
		struct connection* c = &g->conn[i];
		if (c->id >= 0) {
			ls_socket_close(ctx, c->id);
		}
	}
	if (g->listen_id >= 0) {
		ls_socket_close(ctx, g->listen_id);
	}
	messagepool_free(&g->mp);
	hashid_clear(&g->hash);
	ls_free(g->conn);
	ls_free(g);
}

static void
_parm(char* msg, int sz, int command_sz) {
	while (command_sz < sz) {
		if (msg[command_sz] != ' ')
			break;
		++command_sz;
	}
	int i;
	for (i = command_sz; i < sz; i++) {
		msg[i - command_sz] = msg[i];
	}
	msg[i - command_sz] = '\0';
}

static void
_forward_agent(struct gate* g, int fd, uint32_t agentaddr, uint32_t clientaddr) {
	int id = hashid_lookup(&g->hash, fd);
	if (id >= 0) {
		struct connection* agent = &g->conn[id];
		agent->agent = agentaddr;
		agent->client = clientaddr;
	}
}

static void
_ctrl(struct gate* g, const void* msg, int sz) {
	struct ls_context* ctx = g->ctx;
	char* tmp = (char*)ls_malloc(sz + 1);
	memcpy(tmp, msg, sz);
	tmp[sz] = '\0';
	char* command = tmp;
	int i;
	if (sz == 0)
		return;
	for (i = 0; i < sz; i++) {
		if (command[i] == ' ') {
			break;
		}
	}
	if (memcmp(command, "kick", i) == 0) {
		_parm(tmp, sz, i);
		int uid = strtol(command, NULL, 10);
		int id = hashid_lookup(&g->hash, uid);
		if (id >= 0) {
			ls_socket_close(ctx, uid);
		}
		return;
	}
	if (memcmp(command, "forward", i) == 0) {
		_parm(tmp, sz, i);
		char* client = tmp;
		char* idstr = strsep_(&client, " ");
		if (client == NULL) {
			return;
		}
		int id = strtol(idstr, NULL, 10);
		char* agent = strsep_(&client, " ");
		if (client == NULL) {
			return;
		}
		uint32_t agent_handle = strtoul(agent + 1, NULL, 16);
		uint32_t client_handle = strtoul(client + 1, NULL, 16);
		_forward_agent(g, id, agent_handle, client_handle);
		return;
	}
	if (memcmp(command, "broker", i) == 0) {
		_parm(tmp, sz, i);
		g->broker = ls_queryname(ctx, command);
		return;
	}
	if (memcmp(command, "start", i) == 0) {
		_parm(tmp, sz, i);
		int uid = strtol(command, NULL, 10);
		int id = hashid_lookup(&g->hash, uid);
		if (id >= 0) {
			ls_socket_start(ctx, uid);
		}
		return;
	}
	if (memcmp(command, "close", i) == 0) {
		if (g->listen_id >= 0) {
			ls_socket_close(ctx, g->listen_id);
			g->listen_id = -1;
		}
		return;
	}
	ls_error(ctx, "[gate] Unkown command : %s", command);
}

static void
_report(struct gate* g, const char* data, ...) {
	if (g->watchdog == 0) {
		return;
	}
	struct ls_context* ctx = g->ctx;
	va_list ap;
	va_start(ap, data);
	char tmp[1024];
	int n = vsnprintf(tmp, sizeof(tmp), data, ap);
	va_end(ap);

	ls_send(ctx, 0, g->watchdog, PTYPE_TEXT, 0, tmp, n);
}

static void
_forward(struct gate* g, struct connection* c, int size) {
	struct ls_context* ctx = g->ctx;
	int fd = c->id;
	if (fd <= 0) {
		// socket error
		return;
	}
	if (g->broker) {
		void* temp = ls_malloc(size);
		databuffer_read(&c->buffer, &g->mp, temp, size);
		ls_send(ctx, 0, g->broker, g->client_tag | PTYPE_TAG_DONTCOPY, fd, temp, size);
		return;
	}
	if (c->agent) {
		void* temp = ls_malloc(size);
		databuffer_read(&c->buffer, &g->mp, temp, size);
		ls_send(ctx, c->client, c->agent, g->client_tag | PTYPE_TAG_DONTCOPY, fd, temp, size);
	}
	else if (g->watchdog) {
		char* tmp = (char*)ls_malloc(size + 32);
		int n = snprintf(tmp, 32, "%d data ", c->id);
		databuffer_read(&c->buffer, &g->mp, tmp + n, size);
		ls_send(ctx, 0, g->watchdog, PTYPE_TEXT | PTYPE_TAG_DONTCOPY, fd, tmp, size + n);
	}
}

static void
dispatch_message(struct gate* g, struct connection* c, int id, void* data, int sz) {
	databuffer_push(&c->buffer, &g->mp, data, sz);
	for (;;) {
		int size = databuffer_readheader(&c->buffer, &g->mp, g->header_size);
		if (size < 0) {
			return;
		}
		else if (size > 0) {
			if (size >= 0x1000000) {
				struct ls_context* ctx = g->ctx;
				databuffer_clear(&c->buffer, &g->mp);
				ls_socket_close(ctx, id);
				ls_error(ctx, "Recv socket message > 16M");
				return;
			}
			else {
				_forward(g, c, size);
				databuffer_reset(&c->buffer);
			}
		}
	}
}

static void
dispatch_socket_message(struct gate* g, const struct ls_socket_message* message, int sz) {
	struct ls_context* ctx = g->ctx;
	switch (message->type) {
	case LS_SOCKET_TYPE_DATA: {
		int id = hashid_lookup(&g->hash, message->id);
		if (id >= 0) {
			struct connection* c = &g->conn[id];
			dispatch_message(g, c, message->id, message->buffer, message->ud);
		}
		else {
			ls_error(ctx, "Drop unknown connection %d message", message->id);
			ls_socket_close(ctx, message->id);
			ls_free(message->buffer);
		}
		break;
	}
	case LS_SOCKET_TYPE_CONNECT: {
		if (message->id == g->listen_id) {
			// start listening
			break;
		}
		int id = hashid_lookup(&g->hash, message->id);
		if (id < 0) {
			ls_error(ctx, "Close unknown connection %d", message->id);
			ls_socket_close(ctx, message->id);
		}
		break;
	}
	case LS_SOCKET_TYPE_CLOSE:
	case LS_SOCKET_TYPE_ERROR: {
		int id = hashid_remove(&g->hash, message->id);
		if (id >= 0) {
			struct connection* c = &g->conn[id];
			databuffer_clear(&c->buffer, &g->mp);
			memset(c, 0, sizeof(*c));
			c->id = -1;
			_report(g, "%d close", message->id);
		}
		break;
	}
	case LS_SOCKET_TYPE_ACCEPT:
		// report accept, then it will be get a LS_SOCKET_TYPE_CONNECT message
		assert(g->listen_id == message->id);
		if (hashid_full(&g->hash)) {
			ls_socket_close(ctx, message->ud);
		}
		else {
			struct connection* c = &g->conn[hashid_insert(&g->hash, message->ud)];
			if (sz >= sizeof(c->remote_name)) {
				sz = sizeof(c->remote_name) - 1;
			}
			c->id = message->ud;
			memcpy(c->remote_name, message + 1, sz);
			c->remote_name[sz] = '\0';
			_report(g, "%d open %d %s:0", c->id, c->id, c->remote_name);
			ls_error(ctx, "socket open: %x", c->id);
		}
		break;
	case LS_SOCKET_TYPE_WARNING:
		ls_error(ctx, "fd (%d) send buffer (%d)K", message->id, message->ud);
		break;
	}
}

static int
_cb(struct ls_context* ctx, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz) {
	struct gate* g = (struct gate*)ud;
	switch (type) {
	case PTYPE_TEXT:
		_ctrl(g, msg, (int)sz);
		break;
	case PTYPE_CLIENT: {
		if (sz <= 4) {
			ls_error(ctx, "Invalid client message from %x", source);
			break;
		}
		// The last 4 bytes in msg are the id of socket, write following bytes to it
		const uint8_t* idbuf = (const uint8_t*)msg + sz - 4;
		uint32_t uid = idbuf[0] | idbuf[1] << 8 | idbuf[2] << 16 | idbuf[3] << 24;
		int id = hashid_lookup(&g->hash, uid);
		if (id >= 0) {
			// don't send id (last 4 bytes)
			ls_socket_send(ctx, uid, (void*)msg, sz - 4);
			// return 1 means don't free msg
			return 1;
		}
		else {
			ls_error(ctx, "Invalid client id %d from %x", (int)uid, source);
			break;
		}
	}
	case PTYPE_SOCKET:
		// recv socket message from ls_socket
		dispatch_socket_message(g, (const ls_socket_message*)msg, (int)(sz - sizeof(struct ls_socket_message)));
		break;
	}
	return 0;
}

static int
start_listen(struct gate* g, char* listen_addr) {
	struct ls_context* ctx = g->ctx;
	char* portstr = strrchr(listen_addr, ':');
	const char* host = "";
	int port;
	if (portstr == NULL) {
		port = strtol(listen_addr, NULL, 10);
		if (port <= 0) {
			ls_error(ctx, "Invalid gate address %s", listen_addr);
			return 1;
		}
	}
	else {
		port = strtol(portstr + 1, NULL, 10);
		if (port <= 0) {
			ls_error(ctx, "Invalid gate address %s", listen_addr);
			return 1;
		}
		portstr[0] = '\0';
		host = listen_addr;
	}
	g->listen_id = ls_socket_listen(ctx, host, port, BACKLOG);
	if (g->listen_id < 0) {
		return 1;
	}
	ls_socket_start(ctx, g->listen_id);
	return 0;
}

int ls_gateway_init(void* g_, struct ls_context* ctx, const char* parm) {
	struct gate* g = (struct gate*)g_;
	if (parm == NULL)
		return 1;
	int max = 0;
	int sz = strlen(parm) + 1;
	char* watchdog = (char*)ls_malloc(sz);
	char* binding = (char*)ls_malloc(sz);
	int client_tag = 0;
	char header;
	int n = sscanf(parm, "%c %s %s %d %d", &header, watchdog, binding, &client_tag, &max);
	if (n < 4) {
		ls_error(ctx, "Invalid gate parm %s", parm);
		return 1;
	}
	if (max <= 0) {
		ls_error(ctx, "Need max connection");
		return 1;
	}
	if (header != 'S' && header != 'L') {
		ls_error(ctx, "Invalid data header style");
		return 1;
	}

	if (client_tag == 0) {
		client_tag = PTYPE_CLIENT;
	}
	if (watchdog[0] == '!') {
		g->watchdog = 0;
	}
	else {
		g->watchdog = ls_queryname(ctx, watchdog);
		if (g->watchdog == 0) {
			ls_error(ctx, "Invalid watchdog %s", watchdog);
			return 1;
		}
	}

	g->ctx = ctx;

	hashid_init(&g->hash, max);
	g->conn = (struct connection*)ls_malloc(max * sizeof(struct connection));
	memset(g->conn, 0, max * sizeof(struct connection));
	g->max_connection = max;
	int i;
	for (i = 0; i < max; i++) {
		g->conn[i].id = -1;
	}

	g->client_tag = client_tag;
	g->header_size = header == 'S' ? 2 : 4;

	ls_callback(ctx, g, _cb);

	return start_listen(g, binding);
}
