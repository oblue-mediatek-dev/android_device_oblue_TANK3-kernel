/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_EXTABLE_H
#define __ASM_EXTABLE_H



struct exception_table_entry
{
	int insn, fixup;
};

#define ARCH_HAS_RELATIVE_EXTABLE

#ifdef CONFIG_BPF_JIT
int arm64_bpf_fixup_exception(const struct exception_table_entry *ex,
			      struct pt_regs *regs);
#else 
static inline
int arm64_bpf_fixup_exception(const struct exception_table_entry *ex,
			      struct pt_regs *regs)
{
	return 0;
}
#endif 

extern int fixup_exception(struct pt_regs *regs);
#endif
