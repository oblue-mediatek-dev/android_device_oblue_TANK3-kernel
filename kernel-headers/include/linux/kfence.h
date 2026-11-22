/* SPDX-License-Identifier: GPL-2.0 */


#ifndef _LINUX_KFENCE_H
#define _LINUX_KFENCE_H

#include <linux/mm.h>
#include <linux/types.h>

#ifdef CONFIG_KFENCE


#define KFENCE_POOL_SIZE ((CONFIG_KFENCE_NUM_OBJECTS + 1) * 2 * PAGE_SIZE)
extern char *__kfence_pool;

#ifdef CONFIG_KFENCE_STATIC_KEYS
#include <linux/static_key.h>
DECLARE_STATIC_KEY_FALSE(kfence_allocation_key);
#else
#include <linux/atomic.h>
extern atomic_t kfence_allocation_gate;
#endif


static __always_inline bool is_kfence_address(const void *addr)
{
	
	return unlikely((unsigned long)((char *)addr - __kfence_pool) < KFENCE_POOL_SIZE && __kfence_pool);
}


void __init kfence_alloc_pool(void);


void __init kfence_init(void);


void kfence_shutdown_cache(struct kmem_cache *s);


void *__kfence_alloc(struct kmem_cache *s, size_t size, gfp_t flags);


static __always_inline void *kfence_alloc(struct kmem_cache *s, size_t size, gfp_t flags)
{
#ifdef CONFIG_KFENCE_STATIC_KEYS
	if (static_branch_unlikely(&kfence_allocation_key))
#else
	if (unlikely(!atomic_read(&kfence_allocation_gate)))
#endif
		return __kfence_alloc(s, size, flags);
	return NULL;
}


size_t kfence_ksize(const void *addr);


void *kfence_object_start(const void *addr);


void __kfence_free(void *addr);


static __always_inline __must_check bool kfence_free(void *addr)
{
	if (!is_kfence_address(addr))
		return false;
	__kfence_free(addr);
	return true;
}


bool __must_check kfence_handle_page_fault(unsigned long addr, bool is_write, struct pt_regs *regs);

#else 

static inline bool is_kfence_address(const void *addr) { return false; }
static inline void kfence_alloc_pool(void) { }
static inline void kfence_init(void) { }
static inline void kfence_shutdown_cache(struct kmem_cache *s) { }
static inline void *kfence_alloc(struct kmem_cache *s, size_t size, gfp_t flags) { return NULL; }
static inline size_t kfence_ksize(const void *addr) { return 0; }
static inline void *kfence_object_start(const void *addr) { return NULL; }
static inline void __kfence_free(void *addr) { }
static inline bool __must_check kfence_free(void *addr) { return false; }
static inline bool __must_check kfence_handle_page_fault(unsigned long addr, bool is_write,
							 struct pt_regs *regs)
{
	return false;
}

#endif

#endif 
