/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __MTK_IDLE_MCDI_COMMON_H__
#define __MTK_IDLE_MCDI_COMMON_H__

#if defined(CONFIG_MACH_MT6763) || defined(CONFIG_MACH_MT6739) \
	|| defined(CONFIG_MACH_MT8183)

#include "spm_v4/mtk_idle_mcdi.h"

#elif defined(CONFIG_MACH_MT6758) || defined(CONFIG_MACH_MT6775)

#include "spm_v3/mtk_idle_mcdi.h"

#endif

#endif /* __MTK_IDLE_MCDI_COMMON_H__ */

