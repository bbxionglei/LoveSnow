#include "framework.h"
#include "ls_base.h"
typedef void* (*ls_dl_create)(void);
typedef int (*ls_dl_init)(void* inst);
typedef void (*ls_dl_release)(void* inst);
typedef void (*ls_dl_signal)(void* inst, int signal);

struct ls_module {
	const char* name;
	void* module;
	ls_dl_create create;
	ls_dl_init init;
	ls_dl_release release;
	ls_dl_signal signal;
};

extern "C" {

LSBASE_API struct ls_module* ls_module_query(const char* path,const char* name);

}
