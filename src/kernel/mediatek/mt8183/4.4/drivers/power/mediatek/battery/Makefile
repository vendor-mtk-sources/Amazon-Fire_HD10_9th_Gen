#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#


MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/pmic/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/pmic/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/power/mediatek/misc
subdir-ccflags-y += -Werror -I$(srctree)/drivers/power/mediatek/charger
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/usb_c/tcpc/inc/

ifeq ($(CONFIG_MTK_GAUGE_VERSION),30)
	obj-y += mtk_battery.o mtk_power_misc.o mtk_gauge_coulomb_service.o mtk_gauge_class.o mtk_battery_recovery.o mtk_battery_core.o
ifeq ($(or $(strip $(CONFIG_AMZN_METRICS_LOG)), $(strip $(CONFIG_AMZN_MINERVA_METRICS_LOG))),y)
	obj-y += battery_metrics.o
endif
ifeq ($(or $(strip $(CONFIG_AMAZON_METRICS_LOG)), $(strip $(CONFIG_AMAZON_MINERVA_METRICS_LOG))),y)
	obj-y += battery_metrics.o
endif
endif

