#ifndef LS_SPINLOCK_H
#define LS_SPINLOCK_H
#include <atomic>

#define SPIN_INIT(q) spinlock_init(&((q)->lock));
#define SPIN_LOCK(q) spinlock_lock(&((q)->lock));
#define SPIN_TRY_LOCK(q) spinlock_trylock(&((q)->lock));
#define SPIN_UNLOCK(q) spinlock_unlock(&((q)->lock));
#define SPIN_DESTROY(q) spinlock_destroy(&((q)->lock));
#define SPIN_AUTO(q) auto_spinlock _simple_auto_spin_lock(&((q)->lock));


struct spinlock {
	std::atomic_flag m;
};
static inline void
	spinlock_init(struct spinlock* lock) {
	lock->m.clear();
}

static inline void
	spinlock_lock(struct spinlock* lock) {
	while (lock->m.test_and_set()) {}
}

static inline int
	spinlock_trylock(struct spinlock* lock) {
	return lock->m.test_and_set();
}

static inline void
	spinlock_unlock(struct spinlock* lock) {
	lock->m.clear();
}

static inline void
spinlock_destroy(struct spinlock* lock) {
}
class auto_spinlock {
public:
	auto_spinlock(struct spinlock* lock) {
		m_lock = lock;
		spinlock_lock(m_lock);
	}
	~auto_spinlock() {
		spinlock_unlock(m_lock);
	}
private:
	struct spinlock* m_lock;
};
#endif//LS_SPINLOCK_H
