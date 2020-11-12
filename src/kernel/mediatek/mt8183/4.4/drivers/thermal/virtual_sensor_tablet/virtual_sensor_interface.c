/*
 * Copyright (C) 2020 Amazon.com, Inc.  All rights reserved.
 * Author: Akwasi Boateng <boatenga@lab126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#include "mt-plat/mtk_thermal_monitor.h"
#include <linux/thermal_framework.h>
#include <linux/power_supply.h>
#include "mach/mtk_thermal.h"
#include "tscpu_settings.h"
#include <tspmic_settings.h>
#include "mt-plat/mtk_wcn_cmb_stub.h"
#include "mach/mtk_ppm_api.h"
#include <ap_thermal_limit.h>
#include <mt-plat/mtk_charger.h>

void custom_ppm_cpu_thermal_protect(unsigned int budget);

#define WPC_NAME "Wireless"
#define BATTERY_NAME "battery"
#define OFFLINE_TEMP 25000 /* 25.000 degree Celsius */

static struct power_supply *bat_psy;
static struct power_supply *wpc_psy;

#define MAXIMUM_CPU_POWER 4600

/*
 * =============================================================
 * Weak functions
 * =============================================================
 */
int __attribute__ ((weak)) get_immediate_gpu_wrap(void)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}

int __attribute__ ((weak)) get_hw_bts_temp_export(int aux_channel)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}

int __attribute__ ((weak)) mtktscharger_get_hw_temp(void)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}

int __attribute__ ((weak)) mtk_wcn_cmb_stub_query_ctrl(void)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}

int __attribute__ ((weak)) tscpu_get_temp_by_bank(
	enum thermal_bank_name ts_bank)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}

int __attribute__ ((weak)) mtktsbattery_get_hw_temp(void)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}

int __attribute__ ((weak)) mtktspmic_get_hw_temp(void)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}

void __attribute__ ((weak))
custom_ppm_cpu_thermal_protect(unsigned int limited_power)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
}

int __attribute__ ((weak)) setMaxbrightness(int max_level, int enable)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}

int __attribute__ ((weak)) charger_manager_enable_dcap(
	struct charger_consumer *consumer, int idx, bool en)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}
/* ============================================================= */

int vs_wpc_read_temp(void)
{
	union power_supply_propval val;
	int ret;

	if (!wpc_psy) {
		wpc_psy = power_supply_get_by_name(WPC_NAME);
		if (!wpc_psy) {
			pr_err("%s: get power supply %s failed\n",
				__func__, WPC_NAME);
			return OFFLINE_TEMP;
		}
	}

	ret = power_supply_get_property(wpc_psy, POWER_SUPPLY_PROP_TEMP, &val);
	if (ret) {
		pr_debug("E_WF: %s device %s is not ready\n",
			__func__, WPC_NAME);
		return OFFLINE_TEMP;
	}

	/* Convert tenths of degree Celsius to milli degree Celsius. */
	return val.intval * 1000;
}

static int set_charging_level_limit(int level_limit,
		enum power_supply_property property)
{
	int ret = 0;
	union power_supply_propval propval;

	if (!bat_psy) {
		bat_psy = power_supply_get_by_name(BATTERY_NAME);
		if (!bat_psy) {
			pr_err("%s: get power supply %s failed\n",
				__func__, BATTERY_NAME);
			return -1;
		}
	}

	propval.intval = level_limit;
	ret = power_supply_set_property(bat_psy, property, &propval);
	if (ret < 0)
		pr_err("%s: VS set psy charging level_limit=%d failed, ret = %d\n",
			__func__, level_limit, ret);
	return ret;
}

void set_cpu_power_limit(int power_limit)
{
	custom_ppm_cpu_thermal_protect(power_limit);
}

/* Get the current temperature of the thermal sensor. */
int vs_thermal_sensor_get_temp(enum vs_thermal_sensor_id id, int index)
{
	switch (id) {
	case VS_THERMAL_SENSOR_CPU:
		return tscpu_get_temp_by_bank(THERMAL_BANK0);
	case VS_THERMAL_SENSOR_PMIC:
		return mtktspmic_get_hw_temp();
	case VS_THERMAL_SENSOR_BATTERY:
		return mtktsbattery_get_hw_temp();
	case VS_THERMAL_SENSOR_WMT:
		return mtk_wcn_cmb_stub_query_ctrl() * 1000;
	case VS_THERMAL_SENSOR_GPU:
		return get_immediate_gpu_wrap();
	case VS_THERMAL_SENSOR_THERMISTOR:
		return get_hw_bts_temp_export(index) * 1000;
	case VS_THERMAL_SENSOR_WIRELESS_CHG:
		return vs_wpc_read_temp();
	case VS_THERMAL_SENSOR_CHARGER:
		return mtktscharger_get_hw_temp();
	default:
		pr_err("E_WF: %s id %d doesn't exist thermal sensor\n",
			__func__, id);
		return OFFLINE_TEMP;
	}
}

/* Set a level limit via the thermal cooler. */
int vs_set_cooling_level(struct thermal_cooling_device *cdev,
	enum vs_thermal_cooler_id id, int level_limit)
{
	struct vs_cooler_platform_data *pdata;

	if (!cdev || !(cdev->devdata)) {
		pr_err("%s cdev: %p or cdev->devdata is NULL!\n", __func__, cdev);
		return -EINVAL;
	}
	pdata = cdev->devdata;

	switch (id) {
	case VS_THERMAL_COOLER_BUDGET:
		set_cpu_power_limit(
			(level_limit < MAXIMUM_CPU_POWER) ? level_limit : 0);
		return 0;
	case VS_THERMAL_COOLER_BCCT:
		return set_charging_level_limit(
			(level_limit >= pdata->max_level) ? -1 : level_limit * 10,
				POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT);
	case VS_THERMAL_COOLER_BACKLIGHT:
		return setMaxbrightness(level_limit, 1);
	case VS_THERMAL_COOLER_WIRELESS_CHG:
		return set_charging_level_limit(
			(level_limit >= pdata->max_level) ? -1 : level_limit,
				POWER_SUPPLY_PROP_THERMAL_INPUT_POWER_LIMIT);
	default:
		pr_err("E_WF: %s doesn't exist thermal cooler\n",
			__func__);
		return -1;
	}
}

int set_shutdown_enable_dcap(struct device *dev)
{
	struct charger_consumer *pthermal_consumer = NULL;

	if (!dev) {
		pr_err("%s dev is NULL!\n", __func__);
		return -1;
	}

	pthermal_consumer = charger_manager_get_by_name(dev, "charger");
	if (pthermal_consumer)
		charger_manager_enable_dcap(pthermal_consumer, 0, true);
	else
		pr_err("%s pthermal_consumer is not ready!\n", __func__);

	return 0;
}
