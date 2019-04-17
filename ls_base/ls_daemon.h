#ifndef ls_daemon_h
#define ls_daemon_h
#include "framework.h"
extern "C" {
	LSBASE_API int daemon_init(const char* pidfile);
	LSBASE_API int daemon_exit(const char* pidfile);
}
#endif
