/* SPDX-License-Identifier: GPL-2.0-only */


#ifndef __ARM64_S2_PGTABLE_H_
#define __ARM64_S2_PGTABLE_H_

#include <linux/pgtable.h>


#define pt_levels_pgdir_shift(lvls)	ARM64_HW_PGTABLE_LEVEL_SHIFT(4 - (lvls))


#define stage2_pgtable_levels(ipa)	ARM64_HW_PGTABLE_LEVELS((ipa) - 4)
#define kvm_stage2_levels(kvm)		VTCR_EL2_LVLS(kvm->arch.vtcr)


#define stage2_pgdir_shift(kvm)		pt_levels_pgdir_shift(kvm_stage2_levels(kvm))
#define stage2_pgdir_size(kvm)		(1ULL << stage2_pgdir_shift(kvm))
#define stage2_pgdir_mask(kvm)		~(stage2_pgdir_size(kvm) - 1)


#define kvm_mmu_cache_min_pages(kvm)	(kvm_stage2_levels(kvm) - 1)

static inline phys_addr_t
stage2_pgd_addr_end(struct kvm *kvm, phys_addr_t addr, phys_addr_t end)
{
	phys_addr_t boundary = (addr + stage2_pgdir_size(kvm)) & stage2_pgdir_mask(kvm);

	return (boundary - 1 < end - 1) ? boundary : end;
}

#endif	
