/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM snd_compr

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH trace/hooks

#if !defined(_TRACE_HOOK_SND_COMPR_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_SND_COMPR_H

#include <linux/tracepoint.h>
#include <trace/hooks/vendor_hooks.h>


DECLARE_HOOK(android_vh_snd_compr_use_pause_in_drain,
	TP_PROTO(bool *use_pause_in_drain, bool *leave_draining),
	TP_ARGS(use_pause_in_drain, leave_draining));

#endif 

#include <trace/define_trace.h>

