#ifndef LS_MODULE_H
#define LS_MODULE_H
#include "framework.h"
extern "C" {
	struct ls_context;

	typedef void* (*ls_dl_create)(void);
	typedef int (*ls_dl_init)(void* inst, struct ls_context*, const char* parm);
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
	LSBASE_API void ls_module_insert(struct ls_module* mod);
	LSBASE_API struct ls_module* ls_module_query(const char* name);
	LSBASE_API void* ls_module_instance_create(struct ls_module*);
	LSBASE_API int ls_module_instance_init(struct ls_module*, void* inst, struct ls_context* ctx, const char* parm);
	LSBASE_API void ls_module_instance_release(struct ls_module*, void* inst);
	LSBASE_API void ls_module_instance_signal(struct ls_module*, void* inst, int signal);

	LSBASE_API void ls_module_init(const char* path);
}
#endif
