#include "ls_log.h"
#include "ls_timer.h"
#include "ls.h"
#include "ls_socket.h"
#include <string.h>
#include <time.h>


FILE*
ls_log_open(struct ls_context* ctx, uint32_t handle) {
	const char* logpath = ls_getenv("logpath");
	if (logpath == NULL)
		return NULL;
	size_t sz = strlen(logpath);
	char* tmp = (char*)ls_malloc(sz + 16);
	sprintf(tmp, "%s/%08x.log", logpath, handle);
	FILE * f = fopen(tmp, "ab");
	if (f) {
		uint32_t starttime = ls_starttime();
		uint64_t currenttime = ls_now();
		time_t ti = starttime + currenttime / 100;
		ls_error(ctx, "Open log file %s", tmp);
		fprintf(f, "open time: %u %s", (uint32_t)currenttime, ctime(&ti));
		fflush(f);
	}
	else {
		ls_error(ctx, "Open log file %s fail", tmp);
	}
	ls_free(tmp);
	return f;
}

void
ls_log_close(struct ls_context* ctx, FILE* f, uint32_t handle) {
	ls_error(ctx, "Close log file :%08x", handle);
	fprintf(f, "close time: %u\n", (uint32_t)ls_now());
	fclose(f);
}

static void
log_blob(FILE* f, void* buffer, size_t sz) {
	size_t i;
	uint8_t* buf = (uint8_t*)buffer;
	for (i = 0; i != sz; i++) {
		fprintf(f, "%02x", buf[i]);
	}
}

static void
log_socket(FILE* f, struct ls_socket_message* message, size_t sz) {
	fprintf(f, "[socket] %d %d %d ", message->type, message->id, message->ud);

	if (message->buffer == NULL) {
		const char* buffer = (const char*)(message + 1);
		sz -= sizeof(*message);
		const char* eol = (const char*)memchr(buffer, '\0', sz);
		if (eol) {
			sz = eol - buffer;
		}
		fprintf(f, "[%*s]", (int)sz, (const char*)buffer);
	}
	else {
		sz = message->ud;
		log_blob(f, (void*)message->buffer, sz);
	}
	fprintf(f, "\n");
	fflush(f);
}

void
ls_log_output(FILE* f, uint32_t source, int type, int session, void* buffer, size_t sz) {
	if (type == PTYPE_SOCKET) {
		log_socket(f, (struct ls_socket_message*)buffer, sz);
	}
	else {
		uint32_t ti = (uint32_t)ls_now();
		fprintf(f, ":%08x %d %d %u ", source, type, session, ti);
		log_blob(f, buffer, sz);
		fprintf(f, "\n");
		fflush(f);
	}
}
