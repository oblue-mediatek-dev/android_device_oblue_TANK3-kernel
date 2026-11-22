/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM pci
#define TRACE_INCLUDE_PATH trace/hooks

#if !defined(_TRACE_HOOK_PCI_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_PCI_H
#include <linux/tracepoint.h>
#include <trace/hooks/vendor_hooks.h>


DECLARE_RESTRICTED_HOOK(android_rvh_pci_d3_sleep,
             TP_PROTO(struct pci_dev *dev, unsigned int delay, int *err),
             TP_ARGS(dev, delay, err), 1);

#endif 


#include <trace/define_trace.h>
