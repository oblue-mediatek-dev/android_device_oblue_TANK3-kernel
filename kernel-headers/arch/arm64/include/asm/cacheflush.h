/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_CACHEFLUSH_H
#define __ASM_CACHEFLUSH_H

#include <linux/kgdb.h>
#include <linux/mm.h>


#define PG_dcache_clean PG_arch_1


extern void __flush_icache_range(unsigned long start, unsigned long end);
extern int  invalidate_icache_range(unsigned long start, unsigned long end);
extern void __flush_dcache_area(void *addr, size_t len);
extern void __inval_dcache_area(void *addr, size_t len);
extern void __clean_dcache_area_poc(void *addr, size_t len);
extern void __clean_dcache_area_pop(void *addr, size_t len);
extern void __clean_dcache_area_pou(void *addr, size_t len);
extern long __flush_cache_user_range(unsigned long start, unsigned long end);
extern void sync_icache_aliases(void *kaddr, unsigned long len);

static inline void flush_icache_range(unsigned long start, unsigned long end)
{
	__flush_icache_range(start, end);

	

	
	if (in_dbg_master())
		return;

	kick_all_cpus_sync();
}
#define flush_icache_range flush_icache_range


extern void __dma_map_area(const void *, size_t, int);
extern void __dma_unmap_area(const void *, size_t, int);
extern void __dma_flush_area(const void *, size_t);


extern void copy_to_user_page(struct vm_area_struct *, struct page *,
	unsigned long, void *, const void *, unsigned long);
#define copy_to_user_page copy_to_user_page


#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 1
extern void flush_dcache_page(struct page *);

static __always_inline void __flush_icache_all(void)
{
	if (cpus_have_const_cap(ARM64_HAS_CACHE_DIC))
		return;

	asm("ic	ialluis");
	dsb(ish);
}

int set_memory_valid(unsigned long addr, int numpages, int enable);

int set_direct_map_invalid_noflush(struct page *page);
int set_direct_map_default_noflush(struct page *page);

#include <asm-generic/cacheflush.h>

#endif 
