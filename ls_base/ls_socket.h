#ifndef ls_socket_h
#define ls_socket_h
#include "framework.h"

#include "socket_info.h"

struct ls_context;

#define LS_SOCKET_TYPE_DATA 1
#define LS_SOCKET_TYPE_CONNECT 2
#define LS_SOCKET_TYPE_CLOSE 3
#define LS_SOCKET_TYPE_ACCEPT 4
#define LS_SOCKET_TYPE_ERROR 5
#define LS_SOCKET_TYPE_UDP 6
#define LS_SOCKET_TYPE_WARNING 7

struct ls_socket_message {
	int type;
	int id;
	int ud;
	char* buffer;
};
extern "C" {
	LSBASE_API void ls_socket_init();
	LSBASE_API void ls_socket_exit();
	LSBASE_API void ls_socket_free();
	LSBASE_API int ls_socket_poll();
	LSBASE_API void ls_socket_updatetime();

	LSBASE_API int ls_socket_send(struct ls_context* ctx, int id, void* buffer, int sz);
	LSBASE_API int ls_socket_send_lowpriority(struct ls_context* ctx, int id, void* buffer, int sz);
	LSBASE_API int ls_socket_listen(struct ls_context* ctx, const char* host, int port, int backlog);
	LSBASE_API int ls_socket_connect(struct ls_context* ctx, const char* host, int port);
	LSBASE_API int ls_socket_bind(struct ls_context* ctx, int fd);
	LSBASE_API void ls_socket_close(struct ls_context* ctx, int id);
	LSBASE_API void ls_socket_shutdown(struct ls_context* ctx, int id);
	LSBASE_API void ls_socket_start(struct ls_context* ctx, int id);
	LSBASE_API void ls_socket_nodelay(struct ls_context* ctx, int id);

	LSBASE_API int ls_socket_udp(struct ls_context* ctx, const char* addr, int port);
	LSBASE_API int ls_socket_udp_connect(struct ls_context* ctx, int id, const char* addr, int port);
	LSBASE_API int ls_socket_udp_send(struct ls_context* ctx, int id, const char* address, const void* buffer, int sz);
	LSBASE_API const char* ls_socket_udp_address(struct ls_socket_message*, int* addrsz);

	LSBASE_API struct socket_info* ls_socket_info();
}
#endif
