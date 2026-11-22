/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM mmc_core

#define TRACE_INCLUDE_PATH trace/hooks

#if !defined(_TRACE_HOOK_MMC_CORE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_MMC_CORE_H

#include <linux/tracepoint.h>
#include <trace/hooks/vendor_hooks.h>

#if defined(__GENKSYMS__) || !IS_ENABLED(CONFIG_MMC_SDHCI)
struct sdhci_host;
#else

#include <../drivers/mmc/host/sdhci.h>
#endif

#ifdef __GENKSYMS__
struct mmc_card;
struct mmc_host;
#else

#include <linux/mmc/card.h>

#include <linux/mmc/host.h>
#endif 

DECLARE_HOOK(android_vh_mmc_blk_reset,
	     TP_PROTO(struct mmc_host *host, int err, bool *allow),
	     TP_ARGS(host, err, allow));
DECLARE_HOOK(android_vh_mmc_blk_mq_rw_recovery,
	     TP_PROTO(struct mmc_card *card),
	     TP_ARGS(card));
DECLARE_HOOK(android_vh_sd_update_bus_speed_mode,
	     TP_PROTO(struct mmc_card *card),
	     TP_ARGS(card));
DECLARE_HOOK(android_vh_mmc_attach_sd,
	     TP_PROTO(struct mmc_host *host, u32 ocr, int err),
	     TP_ARGS(host, ocr, err));
DECLARE_HOOK(android_vh_sdhci_get_cd,
	     TP_PROTO(struct sdhci_host *host, bool *allow),
	     TP_ARGS(host, allow));
DECLARE_HOOK(android_vh_mmc_gpio_cd_irqt,
	     TP_PROTO(struct mmc_host *host, bool *allow),
	     TP_ARGS(host, allow));



#endif 

#include <trace/define_trace.h>
