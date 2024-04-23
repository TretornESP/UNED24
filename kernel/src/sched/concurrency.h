#ifndef _CONCURRENCY_H
#define _CONCURRENCY_H

#include <stdint.h>
#include <stdatomic.h>

typedef _Atomic(int) atomic_int_t;
typedef _Atomic(uint64_t) atomic_uint64_t;
typedef _Atomic(uint8_t) atomic_uint8_t;
typedef _Atomic(int) lock_t;

void acquire_lock(volatile lock_t* lock);
void release_lock(volatile lock_t* lock);
int try_acquire_lock(volatile lock_t * lock);
void init_lock(volatile lock_t * lock);

static inline void atomic_sub_u64(volatile atomic_uint64_t * value, uint64_t sub) {
	atomic_fetch_sub_explicit( value, sub, memory_order_relaxed );
}

static inline void atomic_increment(volatile atomic_int_t * value) {
	atomic_fetch_add_explicit( value, 1, memory_order_relaxed );
}

static inline void atomic_decrement(volatile atomic_int_t * value) {
	atomic_fetch_sub_explicit( value, 1, memory_order_relaxed );
}

static inline void atomic_increment_u64(volatile atomic_uint64_t * value) {
	atomic_fetch_add_explicit( value, 1, memory_order_relaxed );
}

static inline void atomic_decrement_u64(volatile atomic_uint64_t * value) {
	atomic_fetch_sub_explicit( value, 1, memory_order_relaxed );
}

static inline void atomic_increment_u8(volatile atomic_uint8_t * value) {
	atomic_fetch_add_explicit( value, 1, memory_order_relaxed );
}

static inline void atomic_decrement_u8(volatile atomic_uint8_t * value) {
	atomic_fetch_sub_explicit( value, 1, memory_order_relaxed );
}
#endif