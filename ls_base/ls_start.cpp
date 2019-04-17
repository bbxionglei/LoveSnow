#include "ls.h"
#include "ls_server.h"
#include "ls_mq.h"
#include "ls_handle.h"
#include "ls_module.h"
#include "ls_timer.h"
#include "ls_monitor.h"
#include "ls_socket.h"
#include "ls_daemon.h"
#include "ls_harbor.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <thread>             // std::thread
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable

struct monitor {
	int count;
	struct ls_monitor** m;
	std::condition_variable cond;
	std::mutex mutex;
	int sleep;
	int quit;
};

struct worker_parm {
	struct monitor* m;
	int id;
	int weight;
};

static int SIG = 0;

static void
handle_hup(int signal) {
#ifdef _WIN32
#else
	if (signal == SIGHUP) {
		SIG = 1;
	}
#endif
}

#define CHECK_ABORT if (ls_context_total()==0) break;//判断是不是所有的服务都运行完了。

static void
wakeup(struct monitor* m, int busy) {
	if (m->sleep >= m->count - busy) {
		// signal sleep worker, "spurious wakeup" is harmless
		std::unique_lock<std::mutex> lck(m->mutex);
		m->cond.notify_all();
	}
}

static void*
thread_socket(void* p) {
	struct monitor* m = (struct monitor*)p;
	ls_initthread(THREAD_SOCKET);
	for (;;) {
		int r = ls_socket_poll();
		if (r == 0)
			break;
		if (r < 0) {
			CHECK_ABORT
				continue;
		}
		wakeup(m, 0);
	}
	return NULL;
}

static void
free_monitor(struct monitor* m) {
	int i;
	int n = m->count;
	for (i = 0; i < n; i++) {
		ls_monitor_delete(m->m[i]);
	}
	ls_free(m->m);
	ls_free(m);
}

static void*
thread_monitor(void* p) {
	struct monitor* m = (struct monitor*)p;
	int i;
	int n = m->count;
	ls_initthread(THREAD_MONITOR);
	for (;;) {
		CHECK_ABORT
			for (i = 0; i < n; i++) {
				ls_monitor_check(m->m[i]);
			}
		//为什么设置成5次循环sleep
		for (i = 0; i < 5; i++) {
			CHECK_ABORT
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}

	return NULL;
}

static void
signal_hup() {
	// make log file reopen

	struct ls_message smsg;
	smsg.source = 0;
	smsg.session = 0;
	smsg.data = NULL;
	smsg.sz = (size_t)PTYPE_SYSTEM << MESSAGE_TYPE_SHIFT;
	uint32_t logger = ls_handle_findname("logger");
	if (logger) {
		ls_context_push(logger, &smsg);
	}
}

static void*
thread_timer(void* p) {
	struct monitor* m = (struct monitor*)p;
	ls_initthread(THREAD_TIMER);
	for (;;) {
		ls_updatetime();
		ls_socket_updatetime();
		CHECK_ABORT
			wakeup(m, m->count - 1);
		//usleep(2500);//0.0025秒 2.5毫秒 2500微秒 
		std::this_thread::sleep_for(std::chrono::microseconds(2500));
		if (SIG) {
			signal_hup();
			SIG = 0;
		}
	}
	// wakeup socket thread
	ls_socket_exit();
	// wakeup all worker thread
	std::unique_lock<std::mutex> lck(m->mutex);
	m->quit = 1;
	m->cond.notify_all();
	return NULL;
}

// 调度线程的工作函数
static void*
thread_worker(void* p) {
	struct worker_parm* wp = (struct worker_parm*)p;
	int id = wp->id;
	int weight = wp->weight;
	struct monitor* m = wp->m;
	struct ls_monitor* sm = m->m[id];
	ls_initthread(THREAD_WORKER);
	struct message_queue* q = NULL;
	while (!m->quit) {
		q = ls_context_message_dispatch(sm, q, weight); // 消息队列的派发和处理
		if (q == NULL) {
			std::unique_lock<std::mutex> lck(m->mutex);
			++m->sleep;
			// "spurious wakeup" is harmless,
			// because ls_context_message_dispatch() can be call at any time.
			if (!m->quit)
				m->cond.wait(lck);
			--m->sleep;
		}
	}
	return NULL;
}

static void
start(int threadc) {
	std::thread* pid = new std::thread[threadc + 3];

	struct monitor* m = (struct monitor*)ls_malloc(sizeof(*m));
	memset(m, 0, sizeof(*m));
	m = new (m) struct monitor();
	m->count = threadc;//监控几个线程，判断有没有死循环
	m->sleep = 0;
	m->mutex.lock();
	m->mutex.unlock();

	m->m = (struct ls_monitor**)ls_malloc(threadc * sizeof(struct ls_monitor*));
	int i;
	for (i = 0; i < threadc; i++) {
		m->m[i] = ls_monitor_new();
	}

	pid[0] = std::thread(thread_monitor, m);
	pid[1] = std::thread(thread_timer, m);
	pid[2] = std::thread(thread_socket, m);

	static int weight[] = {
		-1, -1, -1, -1, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2,
		3, 3, 3, 3, 3, 3, 3, 3, };
	struct worker_parm* wp = new struct worker_parm[threadc];
	for (i = 0; i < threadc; i++) {
		wp[i].m = m;
		wp[i].id = i;
		if (i < sizeof(weight) / sizeof(weight[0])) {
			wp[i].weight = weight[i];
		}
		else {
			wp[i].weight = 0;
		}
		pid[i + 3] = std::thread(thread_worker, &wp[i]);
	}

	for (i = 0; i < threadc + 3; i++) {
		pid[i].join();
	}
	free_monitor(m);
}

static void
bootstrap(struct ls_context* logger, const char* cmdline) {
	int sz = strlen(cmdline);
	char* name = (char*)ls_malloc(sz + 1);
	char* args = (char*)ls_malloc(sz + 1);
	sscanf(cmdline, "%s %s", name, args);
	struct ls_context* ctx = ls_context_new(name, args);
	if (ctx == NULL) {
		ls_error(NULL, "Bootstrap error : %s\n", cmdline);
		ls_context_dispatchall(logger);
		exit(1);
	}
}

void
ls_start(struct ls_config* config) {
#ifdef _WIN32
#else
	// register SIGHUP for log file reopen
	struct sigaction sa;
	sa.sa_handler = &handle_hup;
	sa.sa_flags = SA_RESTART;
	sigfillset(&sa.sa_mask);
	sigaction(SIGHUP, &sa, NULL);
#endif

	if (config->daemon) {
		if (daemon_init(config->daemon)) {
			exit(1);
		}
	}
	ls_harbor_init(config->harbor);
	ls_handle_init(config->harbor);
	ls_mq_init();
	ls_module_init(config->module_path);
	ls_timer_init();
	ls_socket_init();
	ls_profile_enable(config->profile);

	struct ls_context* ctx = ls_context_new(config->logservice, config->logger);
	if (ctx == NULL) {
		fprintf(stderr, "Can't launch %s service\n", config->logservice);
		exit(1);
	}

	ls_handle_namehandle(ls_context_handle(ctx), "logger");//关联名字到这个handle

	//以 snlua bootstrap 为例 加载 snlua 并传入参数 bootstrap
	bootstrap(ctx, config->bootstrap);

	//看了半天代码，终于到了开启工作线程的时候，累死我了,可是程序执行到这时1毫秒都用不了
	start(config->thread);

	// harbor_exit may call socket send, so it should exit before socket_free
	ls_harbor_exit();
	ls_socket_free();
	if (config->daemon) {
		daemon_exit(config->daemon);
	}
}
