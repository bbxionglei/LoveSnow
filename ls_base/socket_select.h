#ifndef poll_socket_select_h
#define poll_socket_select_h

#include "framework.h"
#include <winsock2.h>
#include <assert.h>

#define MAX_SOCK_FD 4096
static void* s_sock_ud[MAX_SOCK_FD] = { 0 };

static bool
sp_invalid(int efd) {
	return efd == -1;
}

struct sp_select_st {
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
	int max_sock;
};

static int
sp_create() {
	struct sp_select_st* p = (struct sp_select_st*)malloc(sizeof(struct sp_select_st));
	FD_ZERO(&p->readfds);
	FD_ZERO(&p->writefds);
	FD_ZERO(&p->exceptfds);
	return (int)p;
}

static void
sp_release(int efd) {
	struct sp_select_st* p = (struct sp_select_st*)efd;
	free(p);
}

static int
sp_add(int efd, int sock, void* ud) {
	struct sp_select_st* p = (struct sp_select_st*)efd;
	assert(p != 0);
	if (p->readfds.fd_count < FD_SETSIZE) {
		if (sock < MAX_SOCK_FD) {
			s_sock_ud[sock] = ud;
		}
		FD_SET(sock, &p->readfds);
		//FD_SET(sock, &p->writefds);
		FD_SET(sock, &p->exceptfds);
		if (p->max_sock < sock) {
			p->max_sock = sock;
		}
		return 0;
	}
	else {
		return 1;
	}
}

static void
sp_del(int efd, int sock) {
	struct sp_select_st* p = (struct sp_select_st*)efd;
	assert(p != 0);
	if (sock < MAX_SOCK_FD) {
		s_sock_ud[sock] = NULL;
	}
	FD_CLR(sock, &p->readfds);
	FD_CLR(sock, &p->writefds);
	FD_CLR(sock, &p->exceptfds);
}

static void
sp_write(int efd, int sock, void* ud, bool enable) {
	struct sp_select_st* p = (struct sp_select_st*)efd;
	assert(p != 0);
	if (sock < MAX_SOCK_FD) {
		s_sock_ud[sock] = ud;
	}
	if (p->writefds.fd_count < FD_SETSIZE) {
		FD_SET(sock, &p->writefds);
		if (p->max_sock < sock) {
			p->max_sock = sock;
		}
	}
}

static int
sp_wait(int efd, struct st_event* e, int max) {
	struct sp_select_st* p = (struct sp_select_st*)efd;
	assert(p != 0);
	timeval tv = { 0 };
	tv.tv_usec = 100;
	static struct sp_select_st sp;
	sp.readfds = p->readfds;
	sp.writefds = p->writefds;
	sp.exceptfds = p->exceptfds;
	int32_t n = select(p->max_sock, &sp.readfds, &sp.writefds, &sp.exceptfds, &tv);
	int i, j, k = 0;
	for (i = 0; i < sp.readfds.fd_count && k < max; i++) {
		int sock = sp.readfds.fd_array[i];
		if (sock < MAX_SOCK_FD) {
			e[k].s = s_sock_ud[sock];
		}
		e[k].read = true;
		e[k].write = false;
		e[k].error = false;
		e[k++].eof = false;
	}
	for (i = 0; i < sp.writefds.fd_count && k < max; i++) {
		int sock = sp.writefds.fd_array[i];
		for (j = 0; j < k; j++) {
			if (e[j].s == s_sock_ud[sock] && e[j].s != NULL) {
				e[j].write = true;
				FD_CLR(sock, &p->writefds);
				break;
			}
		}
		if (j == k) {
			if (sock < MAX_SOCK_FD) {
				e[k].s = s_sock_ud[sock];
			}
			e[k].read = false;
			e[k].write = true;
			FD_CLR(sock, &p->writefds);
			e[k].error = false;
			e[k++].eof = false;
		}
	}
	for (i = 0; i < sp.exceptfds.fd_count && k < max; i++) {
		int sock = sp.exceptfds.fd_array[i];
		for (j = 0; j < k; j++) {
			if (e[j].s == s_sock_ud[sock] && e[j].s != NULL) {
				e[j].error = true;
				break;
			}
		}
		if (j == k) {
			if (sock < MAX_SOCK_FD) {
				e[k].s = s_sock_ud[sock];
			}
			e[k].read = false;
			e[k].write = false;
			e[k].error = true;
			e[k++].eof = false;
		}
	}
	return k;
}

static void
sp_nonblocking(int fd) {
	unsigned long iMode = 1;
	ioctlsocket(fd, FIONBIO, &iMode);
}

#endif
