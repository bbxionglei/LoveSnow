#ifndef LS_ATOMIC_H
#define LS_ATOMIC_H

#include <atomic>

#define ATOM_CAS(ptr, oval, nval) (ptr)->compare_exchange_strong(oval, nval) // __sync_bool_compare_and_swap(ptr, oval, nval)
//#define ATOM_CAS_POINTER(ptr, oval, nval)  __sync_bool_compare_and_swap(ptr, oval, nval)
#define ATOM_INC(ptr) ++(*ptr)//__sync_add_and_fetch(ptr, 1)
#define ATOM_FINC(ptr) (*ptr)++//__sync_fetch_and_add(ptr, 1)
#define ATOM_DEC(ptr) --(*ptr)//__sync_sub_and_fetch(ptr, 1)
#define ATOM_FDEC(ptr) (*ptr)--//__sync_fetch_and_sub(ptr, 1)

#define ATOM_FADD(ptr,n) (ptr)->fetch_add(n)//__sync_fetch_and_add(ptr, n)
#define ATOM_FSUB(ptr,n) (ptr)->fetch_sub(n)//__sync_fetch_and_sub(ptr, n)
#define ATOM_FAND(ptr,n) (ptr)->fetch_and(n)//__sync_fetch_and_and(ptr, n)
#endif
