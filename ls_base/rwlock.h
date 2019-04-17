#ifndef LS_RWLOCK_H
#define LS_RWLOCK_H
#include <atomic>

struct rwlock {
	std::atomic<char> write;
	std::atomic<char> read;
};

static inline void
rwlock_init(struct rwlock* lock) {
	lock->write = 0;
	lock->read = 0;
}

static inline void
rwlock_rlock(struct rwlock* lock) {
	for (;;) {
		while (lock->write)
			lock->write.load();
		lock->read++;
		if (lock->write) {
			lock->read--;
		}
		else {
			break;
		}
	}
}

static inline void
rwlock_wlock(struct rwlock* lock) {
	for (;;) {
		while (lock->write)lock->write.load();//其它写线程已经加锁
		while (lock->write == 0)lock->write++;//本线程加锁
		if (lock->write == 1)break;
	}
	while (lock->read)lock->read.load();
}

static inline void
rwlock_wunlock(struct rwlock* lock) {
	lock->write--;
}
static inline void
rwlock_runlock(struct rwlock* lock) {
	lock->read--;
}

#endif//LS_RWLOCK_H
