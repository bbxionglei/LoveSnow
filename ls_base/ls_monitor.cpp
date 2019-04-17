#include "ls.h"

#include "ls_monitor.h"
#include "ls_server.h"
#include "ls.h"
#include "atomic.h"

#include <stdlib.h>
#include <string.h>

struct ls_monitor {
	std::atomic<int> version;
	int check_version;
	uint32_t source;
	uint32_t destination;
};

struct ls_monitor*
	ls_monitor_new() {
	struct ls_monitor* ret = (struct ls_monitor*)ls_malloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));
	return ret;
}

void
ls_monitor_delete(struct ls_monitor* sm) {
	ls_free(sm);
}

void
ls_monitor_trigger(struct ls_monitor* sm, uint32_t source, uint32_t destination) {
	sm->source = source;
	sm->destination = destination;
	ATOM_INC(&sm->version);
}

void
ls_monitor_check(struct ls_monitor* sm) {
	if (sm->version == sm->check_version) {
		if (sm->destination) {
			ls_context_endless(sm->destination);
			int v = sm->version;
			ls_error(NULL, "A message from [ :%08x ] to [ :%08x ] maybe in an endless loop (version = %d)", sm->source, sm->destination, v);
		}
	}
	else {
		sm->check_version = sm->version;
	}
}
