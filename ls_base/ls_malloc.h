#ifndef ls_malloc_h
#define ls_malloc_h
#include "framework.h"
#include <stddef.h>
#include <stdlib.h>

#define ls_malloc malloc
#define ls_calloc calloc
#define ls_realloc realloc
#define ls_free free

extern "C" {
	//void * ls_malloc(size_t sz);
	//void * ls_calloc(size_t nmemb, size_t size);
	//void * ls_realloc(void *ptr, size_t size);
	//void ls_free(void *ptr);
	LSBASE_API char* ls_strdup(const char* str);
	LSBASE_API void* ls_lalloc(void* ptr, size_t osize, size_t nsize);	// use for lua
}
#endif
