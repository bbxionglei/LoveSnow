#ifndef ls_socket_server_h
#define ls_socket_server_h

#include "framework.h"
#include <stdint.h>
#include "socket_info.h"

#define SOCKET_DATA 0
#define SOCKET_CLOSE 1
#define SOCKET_OPEN 2
#define SOCKET_ACCEPT 3
#define SOCKET_ERR 4
#define SOCKET_EXIT 5
#define SOCKET_UDP 6
#define SOCKET_WARNING 7

struct socket_server;

struct socket_message {
	int id;
	uintptr_t opaque;
	int ud;	// for accept, ud is new connection id ; for data, ud is size of data 
	char* data;
};
extern "C" {
	LSBASE_API struct socket_server* socket_server_create(uint64_t time);
	LSBASE_API void socket_server_release(struct socket_server*);
	LSBASE_API void socket_server_updatetime(struct socket_server*, uint64_t time);
	LSBASE_API int socket_server_poll(struct socket_server*, struct socket_message* result, int* more);

	LSBASE_API void socket_server_exit(struct socket_server*);
	LSBASE_API void socket_server_close(struct socket_server*, uintptr_t opaque, int id);
	LSBASE_API void socket_server_shutdown(struct socket_server*, uintptr_t opaque, int id);
	LSBASE_API void socket_server_start(struct socket_server*, uintptr_t opaque, int id);

	// return -1 when error
	LSBASE_API int socket_server_send(struct socket_server*, int id, const void* buffer, int sz);
	LSBASE_API int socket_server_send_lowpriority(struct socket_server*, int id, const void* buffer, int sz);

	// ctrl command below returns id
	LSBASE_API int socket_server_listen(struct socket_server*, uintptr_t opaque, const char* addr, int port, int backlog);
	LSBASE_API int socket_server_connect(struct socket_server*, uintptr_t opaque, const char* addr, int port);
	LSBASE_API int socket_server_bind(struct socket_server*, uintptr_t opaque, int fd);

	// for tcp
	LSBASE_API void socket_server_nodelay(struct socket_server*, int id);

	struct socket_udp_address;

	// create an udp socket handle, attach opaque with it . udp socket don't need call socket_server_start to recv message
	// if port != 0, bind the socket . if addr == NULL, bind ipv4 0.0.0.0 . If you want to use ipv6, addr can be "::" and port 0.
	LSBASE_API int socket_server_udp(struct socket_server*, uintptr_t opaque, const char* addr, int port);
	// set default dest address, return 0 when success
	LSBASE_API int socket_server_udp_connect(struct socket_server*, int id, const char* addr, int port);
	// If the socket_udp_address is NULL, use last call socket_server_udp_connect address instead
	// You can also use socket_server_send 
	LSBASE_API int socket_server_udp_send(struct socket_server*, int id, const struct socket_udp_address*, const void* buffer, int sz);
	// extract the address of the message, struct socket_message * should be SOCKET_UDP
	LSBASE_API const struct socket_udp_address* socket_server_udp_address(struct socket_server*, struct socket_message*, int* addrsz);

	struct socket_object_interface {
		void* (*buffer)(void*);
		int(*size)(void*);
		void(*free)(void*);
	};

	// if you send package sz == -1, use soi.
	LSBASE_API void socket_server_userobject(struct socket_server*, struct socket_object_interface* soi);

	LSBASE_API struct socket_info* socket_server_info(struct socket_server*);
	LSBASE_API int xclose(int fd);
	LSBASE_API int xsetsockopt(int fd, int level, int optname, void* optval, int optlen);
	LSBASE_API int xgetsockopt(int fd, int level, int optname, void* optval, void* optlen);
}
#endif
