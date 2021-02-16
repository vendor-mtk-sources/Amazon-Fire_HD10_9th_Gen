/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/string.h>
#include <linux/regulator/consumer.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/syscore_ops.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#else
#include <string.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#endif

#ifdef CONFIG_AMZN_METRICS_LOG
#include <linux/amzn_metricslog.h>
#endif

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#endif

#define LOG_TAG "LCM-FT8201"

#define LCM_LOGI(fmt, args...)              \
		pr_notice("["LOG_TAG"]" fmt "\n", ##args)
#define LCM_DBG(fmt, args...)               \
		pr_debug("["LOG_TAG"]" fmt "\n", ##args)
#define LCM_ERR(fmt, args...)               \
		pr_err("["LOG_TAG"][ERR]" fmt "\n", ##args)

#define LCM_FUNC_ENTRY(fmt, args...)        \
		pr_notice("["LOG_TAG"]%s entry " fmt "\n", __func__, ##args)
#define LCM_FUNC_EXIT(fmt, args...)         \
		pr_notice("["LOG_TAG"]%s exit " fmt "\n", __func__, ##args)

#define LCM_FUNC_DBG_ENTRY(fmt, args...)    \
		pr_debug("["LOG_TAG"]%s entry " fmt "\n", __func__, ##args)
#define LCM_FUNC_DBG_EXIT(fmt, args...)     \
		pr_debug("["LOG_TAG"]%s exit " fmt "\n", __func__, ##args)


static LCM_UTIL_FUNCS lcm_util = {
	.set_reset_pin = NULL,
	.udelay = NULL,
	.mdelay = NULL,
};

/* (lcm_util.set_reset_pin((v))) */
#define SET_RESET_PIN(v) lcm_set_gpio_output(GPIO_LCD_RST, v)

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

#define dsi_set_cmdq_backlight(para_tbl, size, force_update) \
			lcm_util.dsi_set_cmdq_backlight(para_tbl, size, force_update)
#define dsi_set_cmdq_V4(para_tbl, size, force_update) \
	lcm_util.dsi_set_cmdq_V4(para_tbl, size, force_update)
#define dsi_get_cmdq_V4(para_tbl, size, force_update) \
	lcm_util.dsi_get_cmdq_V4(para_tbl, size, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)	 \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                   \
	lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)               \
	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                    \
	lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)            \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define VID_FT_KD_MANTIX  0x1		/* KD, using FT8201 IC */
#define VID_FT_KD_HSD     0x2		/* KD, using FT8201 IC, remove from EVT*/
#define VID_FT_TG_BOE     0x3		/* TG, using FT8201 IC */
#define VID_FT_STARRY_HSD 0x4		/* STARRY, using FT8201 IC */
#define VID_HX_TG_BOE     0x5		/* TG, using HX83102-p IC */
#define VID_FT_KD_INX     0x7		/* KD, using FT8201 IC, add from EVT */
#define VID_FT_STARRY_BOE 0x0		/* STARRY, using FT8201 IC, add from EVT */

#define VID_NULL          0xff	/* NULL */
#define RSVD_CODE         0x81	/* NULL */

/*
 * power on sequnce: vrf18/AVDD/AVEE -> LP11 -> RESET high
 * the LP11 will be enable after resume_power() and before resume()
 * so   set vrf18/AVDD/AVEE at resume_power() //TRS_ON_PHASE1
 * then set RESET at resume()                 //TRS_ON_PHASE2
*/
#define TRS_SLP_TO_OFF       0
#define TRS_OFF_TO_ON        1
#define TRS_SLP              2
#define TRS_SLP_TO_ON        3    /* sleep out */
#define TRS_OFF              4
#define TRS_ON_PHASE1        5
#define TRS_ON_PHASE2        6
#define TRS_DSB              7
#define TRS_DSB_TO_ON        8
#define TRS_DSB_TO_OFF       9
#define TRS_KEEP_CUR         0xFD /* keep current state */
#define TRS_NOT_PSB          0xFE /* not possible */

#define LCM_OFF             0
#define LCM_ON              1
#define LCM_ON1             2
#define LCM_ON2             3
#define LCM_SLP             4
#define LCM_DSB             5

/* trans_table[old_state][new_state]; */
static int trans_table[][6] = {
/*            LCM_OFF         LCM_ON         LCM_ON1        LCM_ON2        LCM_SLP       LCM_DSB   */
/* LCM_OFF */{TRS_KEEP_CUR,   TRS_OFF_TO_ON, TRS_ON_PHASE1, TRS_NOT_PSB,   TRS_NOT_PSB,  TRS_NOT_PSB},
/* LCM_ON  */{TRS_OFF,        TRS_KEEP_CUR,  TRS_KEEP_CUR,  TRS_KEEP_CUR,  TRS_SLP,      TRS_DSB},
/* LCM_ON1 */{TRS_NOT_PSB,    TRS_NOT_PSB,   TRS_KEEP_CUR,  TRS_ON_PHASE2, TRS_NOT_PSB,  TRS_NOT_PSB},
/* LCM_ON2 */{TRS_OFF,        TRS_KEEP_CUR,  TRS_NOT_PSB,   TRS_KEEP_CUR,  TRS_SLP,      TRS_DSB},
/* LCM_SLP */{TRS_SLP_TO_OFF, TRS_SLP_TO_ON, TRS_KEEP_CUR,  TRS_SLP_TO_ON, TRS_KEEP_CUR, TRS_NOT_PSB},
/* LCM_DSB */{TRS_DSB_TO_OFF, TRS_DSB_TO_ON, TRS_KEEP_CUR,  TRS_DSB_TO_ON, TRS_NOT_PSB,  TRS_KEEP_CUR}
};

const char *lcm_state_str[] = {
	"LCM_OFF",
	"LCM_ON",
	"LCM_ON1",
	"LCM_ON2",
	"LCM_SLP",
	"LCM_DSB",
};

#define LCM_T2  5       /* spec: > 3ms */
#define LCM_T3  8       /* spec: > 5ms, MTK sample is 8 */
#define LCM_T4  2       /* spec: > 100us */
#define LCM_T5  1       /* spec: > 0ms, MTK sample is 10 */
#define LCM_T6  38      /* spec: > 35ms */
#define LCM_T7  350     /* spec: > 350ms */
#define LCM_T10 160     /* spec: > 150ms */
#define LCM_T11 6       /* spec: > 5ms */
#define LCM_T12 6       /* spec: > 5ms */
#define LCM_T13 6       /* spec: 5ms < LCM_T13 < 120ms */
#define LCM_T14 50      /* spec: > 50ms */
#define LCM_T17 160     /* spec: > 150ms */
#define LCM_Trw 3       /* spec: < 10ms */
#define LCM_Trf 6       /* spec: > 5ms */
#define LCM_Trl 6       /* spec: > 5ms */
#define LCM_TSLPOUT 120 /* MTK is 120ms */
#define LCM_TSLP_TO_DSLP 50 /* focal recommand */
#define LCM_Tdb 4       /* display bias ready */
#define LCM_TLP11_TO_RESX 1 /* spec: undefined */

#define LCM_ID1_VENDOR_SHIFT    5
#define LCM_ID_RETRY_CNT        3
#define LCM_AVDD_VOL            6000000
#define LCM_AVEE_VOL            6000000
#define LCM_DELAY_SHUTDOWN_TO_DISPLAY_SHUTDOWN

#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0

#define GetField(Var, Mask, Shift) \
	(((Var) >> (Shift)) & (Mask))

#define SetField(Var, Mask, Shift, Val) \
	((Var) = (((Var) & ~((Mask) << (Shift))) | \
				(((Val) & (Mask)) << (Shift))))

#define REGFLAG_DELAY         0xFFFC
#define REGFLAG_UDELAY        0xFFFB
#define REGFLAG_END_OF_TABLE  0xFFFD

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table sleep_out_setting[] = {
	{0x11, 0, {} },
	{REGFLAG_DELAY, LCM_TSLPOUT, {} },
	{0x29, 0, {} }
};

static struct LCM_setting_table sleep_in_setting[] = {
	{0x10, 0, {} }
};

static struct LCM_setting_table deep_standby_setting[] = {
	{0x04, 1, {0xA5} },
	{0x05, 1, {0xA5} }
};


#define WORKAROUND_FT8201_SETUP_LCM_CABC_THROUGH_TOUCH_I2C

#ifdef WORKAROUND_FT8201_SETUP_LCM_CABC_THROUGH_TOUCH_I2C

#define CABC_ON_VAL  0x03
#define CABC_OFF_VAL 0x00
extern int ft8201_share_cabc_write(u8 mode);
extern int ft8201_share_cabc_read(u8 *mode);

#else

#define	DSI_DCS_SHORT_PACKET_ID_0	0x05
#define	DSI_DCS_SHORT_PACKET_ID_1	0x15
#define	DSI_DCS_LONG_PACKET_ID		0x39
#define	DSI_DCS_READ_PACKET_ID		0x06

static struct LCM_setting_table_V4 cabc_on_setting[] = {
	{DSI_DCS_SHORT_PACKET_ID_1, 0x50, 1, {0x5A}, 0 },
	{DSI_DCS_SHORT_PACKET_ID_1, 0x51, 1, {0x23}, 0 },
	{DSI_DCS_SHORT_PACKET_ID_1, 0x95, 1, {0x03}, 0 },
	{DSI_DCS_SHORT_PACKET_ID_1, 0x51, 1, {0x2F}, 0 },
	{DSI_DCS_SHORT_PACKET_ID_1, 0x50, 1, {0x01}, 0 }
};

static struct LCM_setting_table_V4 cabc_read[] = {
	{DSI_DCS_SHORT_PACKET_ID_1, 0x50, 1, {0x5A}, 0 },
	{DSI_DCS_SHORT_PACKET_ID_1, 0x51, 1, {0x23}, 0 },
	{DSI_DCS_SHORT_PACKET_ID_1, 0x95, 1, {0x03}, 1 },
	{DSI_DCS_SHORT_PACKET_ID_1, 0x51, 1, {0x2F}, 0 },
	{DSI_DCS_SHORT_PACKET_ID_1, 0x50, 1, {0x03}, 0 }
};

static struct LCM_setting_table_V4 cabc_off_setting[] = {
	{DSI_DCS_SHORT_PACKET_ID_1, 0x50, 1, {0x5A}, 0 },
	{DSI_DCS_SHORT_PACKET_ID_1, 0x51, 1, {0x23}, 0 },
	{DSI_DCS_SHORT_PACKET_ID_1, 0x95, 1, {0x00}, 0 },
	{DSI_DCS_SHORT_PACKET_ID_1, 0x51, 1, {0x2F}, 0 },
	{DSI_DCS_SHORT_PACKET_ID_1, 0x50, 1, {0x02}, 0 }
};

#endif

#define BOARD_TYPE_TRONA    0x5E
#define BOARD_REV_PROTO     0x00
#define BOARD_REV_HVT       0x10
#define BOARD_REV_EVT       0x20
#define BOARD_REV_DVT       0x30
#define BOARD_REV_PVT       0x40
extern unsigned int idme_get_board_rev(void);
extern unsigned int idme_get_board_type(void);

static DEFINE_MUTEX(ft8201_share_power_mutex);

static struct regulator *reg_lcm_vcc1v8;
static unsigned int LCM_RST_GPIO;
static unsigned int TP_RST_GPIO;
static unsigned int g_vendor_id = VID_NULL;
static int old_state = LCM_OFF;
static atomic_t g_power_request = ATOMIC_INIT(0);

static void lcm_power_manager(int);
static void lcm_power_manager_lock(int);
static unsigned int get_lcm_id(void);
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output);
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update);

#if defined(CONFIG_AMZN_METRICS_LOG)
static char metric_buf[128];
#endif

/*
* 1. export function
* 2. lcm_platform_driver
* 3. lcm_driver
*/

/* export function sstart */
int ft8201_share_request_powerpin_resource(struct device *dev)
{
	struct device_node *lcm_common_resource = NULL;
	int ret = 0;

	LCM_FUNC_ENTRY();
	mutex_lock(&ft8201_share_power_mutex);
	if (atomic_read(&g_power_request) > 0) {
		LCM_LOGI("lcm resource already init before");
		goto init_done;
	}

	lcm_common_resource = of_parse_phandle(dev->of_node,
			"lcm_common_resource", 0);
	if (!lcm_common_resource)
		goto err_dts_parsing;

	LCM_RST_GPIO = of_get_named_gpio(lcm_common_resource,
			"lcm_rst_gpio", 0);
	ret = gpio_request(LCM_RST_GPIO, "LCM_RST_GPIO");
	if (ret != 0) {
		LCM_ERR("gpio request LCM_RST_GPIO = 0x%x fail with %d",
			LCM_RST_GPIO, ret);
		goto err_lcm_rst;
	}

	TP_RST_GPIO = of_get_named_gpio(lcm_common_resource,
			"tp_rst_gpio", 0);
	ret = gpio_request(TP_RST_GPIO, "TP_RST_GPIO");
	if (ret != 0) {
		LCM_ERR("gpio request TP_RST_GPIO = 0x%x fail with %d",
			TP_RST_GPIO, ret);
		goto err_tp_rst;
	}

	reg_lcm_vcc1v8 = regulator_get(dev, "lcm_common_vcc1v8");
	if (IS_ERR(reg_lcm_vcc1v8)) {
		LCM_ERR("Failed to request lcm_common_vcc1v8 power supply: %ld",
			PTR_ERR(reg_lcm_vcc1v8));
		goto err_vcc1v8_1;
	}

	ret = regulator_enable(reg_lcm_vcc1v8);
	if (ret != 0) {
		LCM_ERR("Failed to enable lcm_vcc1v8 power supply: %d", ret);
		goto err_vcc1v8_2;
	}

	ret = display_bias_regulator_init();
	if (ret != 0) {
		LCM_ERR("Failed to enable display bias supply: %d", ret);
		goto err_disp_bias;
	}

	ret = display_bias_enable_vol(LCM_AVDD_VOL, LCM_AVEE_VOL);
	if (ret != 0) {
		LCM_ERR("Failed to config display bias supply: %d", ret);
		goto err_disp_bias;
	}
	old_state = LCM_ON;

init_done:
	atomic_inc(&g_power_request);
	mutex_unlock(&ft8201_share_power_mutex);
	LCM_FUNC_EXIT("cnt(%d)", atomic_read(&g_power_request));
	return 0;

err_disp_bias:
	regulator_disable(reg_lcm_vcc1v8);
err_vcc1v8_2:
	regulator_put(reg_lcm_vcc1v8);
	reg_lcm_vcc1v8 = NULL;
err_vcc1v8_1:
	gpio_free(TP_RST_GPIO);
err_tp_rst:
	gpio_free(LCM_RST_GPIO);
err_lcm_rst:
err_dts_parsing:
	mutex_unlock(&ft8201_share_power_mutex);
	LCM_FUNC_EXIT("fail to probe, cnt(%d)", atomic_read(&g_power_request));
	return -1;
}
EXPORT_SYMBOL(ft8201_share_request_powerpin_resource);

void ft8201_share_powerpin_on(void)
{
	mutex_lock(&ft8201_share_power_mutex);
	/*LCM resume power with ON1->ON2, touch resume with ON.*/
	if (atomic_read(&g_power_request) == 0)
		LCM_ERR("Touch poweron should NOT before LCM poweron");
	lcm_power_manager(LCM_ON);
	mutex_unlock(&ft8201_share_power_mutex);
}
EXPORT_SYMBOL(ft8201_share_powerpin_on);

void ft8201_share_powerpin_off(void)
{
	mutex_lock(&ft8201_share_power_mutex);
	lcm_power_manager(LCM_OFF);
	mutex_unlock(&ft8201_share_power_mutex);
}
EXPORT_SYMBOL(ft8201_share_powerpin_off);

void ft8201_share_tp_resetpin_high(void)
{
	LCM_LOGI("%s", __func__);
	mutex_lock(&ft8201_share_power_mutex);
	if (atomic_read(&g_power_request) <= 0) {
		LCM_ERR("%s skip, bceause res_cnt(%d) <= 0",
				__func__, atomic_read(&g_power_request));
	} else {
		lcm_set_gpio_output(TP_RST_GPIO, GPIO_OUT_ONE);
	}
	mutex_unlock(&ft8201_share_power_mutex);
}
EXPORT_SYMBOL(ft8201_share_tp_resetpin_high);

void ft8201_share_tp_resetpin_low(void)
{
	LCM_LOGI("%s", __func__);
	mutex_lock(&ft8201_share_power_mutex);
	if (atomic_read(&g_power_request) <= 0) {
		LCM_ERR("%s skip, bceause res_cnt(%d) <= 0",
				__func__, atomic_read(&g_power_request));
	} else {
		lcm_set_gpio_output(TP_RST_GPIO, GPIO_OUT_ZERO);
	}
	mutex_unlock(&ft8201_share_power_mutex);
}
EXPORT_SYMBOL(ft8201_share_tp_resetpin_low);

unsigned int ft8201_incell_compare_id(void)
{
	LCM_LOGI("%s", __func__);

	/* lcm id get from Lk */
	if ((g_vendor_id == VID_FT_KD_HSD) ||
		(g_vendor_id == VID_FT_KD_MANTIX) ||
		(g_vendor_id == VID_FT_STARRY_HSD) ||
		(g_vendor_id == VID_FT_TG_BOE) ||
		(g_vendor_id == VID_FT_KD_INX) ||
		(g_vendor_id == VID_FT_STARRY_BOE))
		return g_vendor_id;
	else
		return VID_NULL;
}
EXPORT_SYMBOL(ft8201_incell_compare_id);

/* export function end */


/* lcm_platform_driver start */
static int lcm_driver_probe(struct device *dev, void const *data)
{
	int ret = 0;

	if (ft8201_incell_compare_id() != VID_NULL) {
		if (ft8201_share_request_powerpin_resource(dev) < 0) {
			LCM_ERR("fail to request power pins, Panel ID = %x",
					g_vendor_id);
			ret = -1;
		} else {
			LCM_LOGI("lcm probe success, Panel ID = %x",
					g_vendor_id);
			ret = 0;
		}
	} else {
		LCM_ERR("lcm probe fail, mismatch Panel ID = %x", g_vendor_id);
		ret = -ENODEV;
	}

	return ret;
}

static const struct of_device_id lcm_platform_of_match[] = {
	{
		.compatible = "focal,ft8201",
		.data = 0,
	}, {
		/* sentinel */
	}
};

MODULE_DEVICE_TABLE(of, platform_of_match);

static int lcm_platform_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;

	LCM_LOGI("%s", __func__);
	id = of_match_node(lcm_platform_of_match, pdev->dev.of_node);
	if (!id)
		return -ENODEV;

	return lcm_driver_probe(&pdev->dev, id->data);
}

static int lcm_platform_remove(struct platform_device *pdev)
{
	LCM_LOGI("%s", __func__);
	return 0;
}

static void lcm_platform_shutdown(struct platform_device *pdev)
{
	LCM_LOGI("%s", __func__);
#ifdef LCM_DELAY_SHUTDOWN_TO_DISPLAY_SHUTDOWN
	mutex_lock(&ft8201_share_power_mutex);
	atomic_dec(&g_power_request);
	mutex_unlock(&ft8201_share_power_mutex);
#endif
}

static struct platform_driver lcm_driver = {
	.probe = lcm_platform_probe,
	.remove = lcm_platform_remove,
	.shutdown = lcm_platform_shutdown,
	.driver = {
		.name = "ft8201_wuxga_dsi_vdo",
		.owner = THIS_MODULE,
		.of_match_table = lcm_platform_of_match,
	},
};

static int __init lcm_platform_init(void)
{
	LCM_DBG("Register panel driver for ft8201_wuxga_dsi_vdo");
	if (platform_driver_register(&lcm_driver)) {
		LCM_ERR("Failed to register this driver!");
		return -ENODEV;
	}

	return 0;
}

static void __exit lcm_platform_exit(void)
{
	platform_driver_unregister(&lcm_driver);
	LCM_DBG("Unregister this driver done");
}

late_initcall(lcm_platform_init);
module_exit(lcm_platform_exit);
MODULE_AUTHOR("Compal");
MODULE_DESCRIPTION("LCM display subsystem driver");
MODULE_LICENSE("GPL");
/* lcm_platform_driver end */


/* lcm_driver start */
#if defined(MTK_ALPS_BOX_SUPPORT)
#define HDMI_SUB_PATH 1
#else
#define HDMI_SUB_PATH 0
#endif

#if HDMI_SUB_PATH
#define FRAME_WIDTH	 (1920)
#define FRAME_HEIGHT (1080)
#else
#define FRAME_WIDTH	 (1200)
#define FRAME_HEIGHT (1920)
#endif

#define LCM_DSI_CMD_MODE			0

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	LCM_DBG("GPIO = 0x%x, Output= 0x%x", GPIO, output);

#ifdef BUILD_LK
	mt_set_gpio_mode(GPIO, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
#else
#if HDMI_SUB_PATH
#else
	gpio_direction_output(GPIO, (output > 0) ?
			GPIO_OUT_ONE : GPIO_OUT_ZERO);
	gpio_set_value(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
#endif
#endif	/* #ifdef BUILD_LK */
}

static void lcm_get_params(LCM_PARAMS *params)
{

	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->lcm_if = LCM_INTERFACE_DSI0;
	params->lcm_cmd_if = LCM_INTERFACE_DSI0;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_EVENT_VDO_MODE;
#endif

	params->dsi.LANE_NUM = LCM_FOUR_LANE;

	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	params->dsi.packet_size = 256;
	params->dsi.intermediat_buffer_num = 0;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count = FRAME_WIDTH * 3;

	if ((g_vendor_id == VID_FT_TG_BOE) ||
		(g_vendor_id == VID_FT_STARRY_BOE)) {
		params->dsi.vertical_sync_active = 8;
		params->dsi.vertical_backporch = 32;
		params->dsi.vertical_frontporch = 177;
		params->dsi.vertical_active_line = FRAME_HEIGHT;

		params->dsi.horizontal_sync_active = 3;
		params->dsi.horizontal_backporch = 10;
		params->dsi.horizontal_frontporch = 15;
		params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	} else {
		params->dsi.vertical_sync_active = 8;
		params->dsi.vertical_backporch = 32;
		params->dsi.vertical_frontporch = 155;
		params->dsi.vertical_active_line = FRAME_HEIGHT;

		params->dsi.horizontal_sync_active = 3;
		params->dsi.horizontal_backporch = 17;
		params->dsi.horizontal_frontporch = 24;
		params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	}
	LCM_LOGI("LCM(id=%x, VFP=%d)", g_vendor_id,
		params->dsi.vertical_frontporch);

	params->dsi.PLL_CLOCK = 497;

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.ssc_disable = 1;
	params->dsi.cont_clock = 1; /* Keep HS serial clock running */
	params->dsi.DA_HS_EXIT = 10;
	params->dsi.CLK_ZERO = 16;
	params->dsi.HS_ZERO = 25;
	params->dsi.HS_TRAIL = 10;
	params->dsi.CLK_TRAIL = 1;
	params->dsi.CLK_HS_POST = 8;
	params->dsi.CLK_HS_EXIT = 6;
	/* params->dsi.CLK_HS_PRPR = 1; */

	params->dsi.TA_GO = 8;
	params->dsi.TA_GET = 10;

	params->dsi.esd_check_enable = 0;
	/*params->dsi.cust_clk_impendence = 0xf;*/

	/* The values are in mm.*/
	/* Add these values for reporting correct xdpi and ydpi values */
	params->physical_width = 135;
	params->physical_height = 217;
}

static unsigned int get_lcm_id(void)
{
	unsigned int vendor_id = VID_NULL;
	unsigned int vendor_id_tmp = VID_NULL;
	unsigned char buffer[5] = {0};
	unsigned int data_array[16];
	unsigned int retry_cnt;
	int skiplcmid0 = 0;

	LCM_FUNC_DBG_ENTRY();
	retry_cnt = LCM_ID_RETRY_CNT;

	data_array[0] = 0x5a501500; /* Stop reload */
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x02511500; /* BANK 0x02 */
	dsi_set_cmdq(data_array, 1, 1);

	while (retry_cnt > 0) {
		data_array[0] = 0x00053700;
		dsi_set_cmdq(data_array, 1, 1);

		read_reg_v2(0x80, buffer, 5);

		LCM_LOGI("Display ID0(%x), ID1(%x), ID2(%x), ID3(%x), ID4(%x)",
				buffer[0], buffer[1], buffer[2],
				buffer[3], buffer[4]);

		vendor_id_tmp = buffer[1] > LCM_ID1_VENDOR_SHIFT;
		LCM_LOGI("vendor_id_tmp = 0x%x", vendor_id_tmp);

		/* ID2(addr=0x81) value is reserved as 0x81. use this to check if it's a ft8201 driver IC */
		skiplcmid0 = (buffer[2] != RSVD_CODE);

		if ((vendor_id_tmp == VID_FT_KD_HSD) ||
			(vendor_id_tmp == VID_FT_KD_MANTIX) ||
			(vendor_id_tmp == VID_FT_STARRY_HSD) ||
			(vendor_id_tmp == VID_FT_TG_BOE) ||
			(vendor_id_tmp == VID_FT_KD_INX) ||
			((vendor_id_tmp == VID_FT_STARRY_BOE) && !skiplcmid0)) {
			vendor_id = vendor_id_tmp;
			LCM_LOGI("vendor_id = 0x%x", vendor_id);
			break;
		}
		retry_cnt--;
		LCM_ERR("read id fail, retry_cnt(%d)",
			retry_cnt);
	}

	data_array[0] = 0x2f511500; /* BANK 0x2F */
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xa5501500; /* Stop reload off */
	dsi_set_cmdq(data_array, 1, 1);

	LCM_FUNC_DBG_EXIT();
	return vendor_id;
}

static void init_lcm_registers(void)
{

	switch (g_vendor_id) {
	case VID_FT_KD_MANTIX:
	case VID_FT_KD_HSD:
	case VID_FT_TG_BOE:
	case VID_FT_STARRY_HSD:
	case VID_FT_KD_INX:
	case VID_FT_STARRY_BOE:
		LCM_LOGI("init lcm(id=%x) registers", g_vendor_id);
		/* vendor confirm no need to set 11 -> 29 to turn on lcm */
		break;

	default:
		LCM_LOGI("Not supported yet, g_vendor_id = 0x%x",
			g_vendor_id);

		break;
	}
}

static void lcm_init_power(void)
{
	LCM_ERR("%s should not be here because lk already done it",
		__func__);
}

static void lcm_init(void)
{
	if (g_vendor_id == VID_NULL)
		g_vendor_id = get_lcm_id();
	LCM_LOGI("%s should not be here because lk already done it",
		__func__);
}

static void lcm_resume_power(void)
{
	LCM_LOGI("%s", __func__);
	lcm_power_manager_lock(LCM_ON1);
}

static void lcm_resume(void)
{
	LCM_LOGI("%s", __func__);
#if defined(CONFIG_AMAZON_METRICS_LOG) || defined(CONFIG_AMZN_METRICS_LOG)
	snprintf(metric_buf, sizeof(metric_buf),
		"%s:lcd:resume=1;CT;1:NR", __func__);
	log_to_metrics(ANDROID_LOG_INFO, "lcd", metric_buf);
#endif
	lcm_power_manager_lock(LCM_ON2);
}

static void lcm_suspend(void)
{
	LCM_LOGI("%s", __func__);
#if defined(CONFIG_AMAZON_METRICS_LOG) || defined(CONFIG_AMZN_METRICS_LOG)
	snprintf(metric_buf, sizeof(metric_buf),
		"%s:lcd:suspend=1;CT;1:NR", __func__);
	log_to_metrics(ANDROID_LOG_INFO, "lcd", metric_buf);
#endif
	lcm_power_manager_lock(LCM_DSB);
}

static void lcm_suspend_power(void)
{
	LCM_LOGI("%s", __func__);
	lcm_power_manager_lock(LCM_OFF);
}

static int lcm_power_controller(int state)
{
	int ret = 0;

	LCM_FUNC_DBG_ENTRY();
	switch (state) {
	case TRS_SLP:
		push_table(sleep_in_setting,
			sizeof(sleep_in_setting) / sizeof(struct LCM_setting_table), 1);
		MDELAY(LCM_T10);
		break;

	case TRS_DSB:
		push_table(sleep_in_setting,
			sizeof(sleep_in_setting) / sizeof(struct LCM_setting_table), 1);
		MDELAY(LCM_TSLP_TO_DSLP);
		push_table(deep_standby_setting,
			sizeof(deep_standby_setting) / sizeof(struct LCM_setting_table), 1);
		MDELAY(LCM_T17);
		break;

	case TRS_SLP_TO_ON:
		push_table(sleep_out_setting,
			sizeof(sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
		break;

	case TRS_DSB_TO_ON:
		lcm_set_gpio_output(TP_RST_GPIO, GPIO_OUT_ZERO);
		MDELAY(LCM_T12);
		lcm_set_gpio_output(LCM_RST_GPIO, GPIO_OUT_ZERO);
		MDELAY(LCM_T13);
		lcm_set_gpio_output(TP_RST_GPIO, GPIO_OUT_ONE);
		MDELAY(LCM_T5);
		lcm_set_gpio_output(LCM_RST_GPIO, GPIO_OUT_ONE);
		MDELAY(LCM_T6);
		init_lcm_registers();
		break;

	case TRS_SLP_TO_OFF:
	case TRS_DSB_TO_OFF:
		ret = display_bias_disable();
		if (ret < 0)
			LCM_ERR("disable display bias power fail: %d",
				ret);
		MDELAY(LCM_T11 + LCM_Tdb);
		if (reg_lcm_vcc1v8) {
			ret = regulator_disable(reg_lcm_vcc1v8);
			if (ret != 0)
				LCM_ERR("disable reg_lcm_vcc1v8 fail");
		}
		lcm_set_gpio_output(TP_RST_GPIO, GPIO_OUT_ZERO);
		lcm_set_gpio_output(LCM_RST_GPIO, GPIO_OUT_ZERO);
		break;

	case TRS_ON_PHASE1:
		if (reg_lcm_vcc1v8) {
			ret = regulator_enable(reg_lcm_vcc1v8);
			if (ret != 0)
				LCM_ERR("enable reg_lcm_vcc1v8 fail");
		}
		MDELAY(LCM_T2);
		ret = display_bias_enable_vol(LCM_AVDD_VOL, LCM_AVEE_VOL);
		if (ret < 0)
			LCM_ERR("enable display bias power fail: %d", ret);
		MDELAY(LCM_T3 - LCM_T2 + LCM_T4 + LCM_Tdb);
		break;

	case TRS_ON_PHASE2:
		MDELAY(LCM_TLP11_TO_RESX);
		lcm_set_gpio_output(TP_RST_GPIO, GPIO_OUT_ONE);
		MDELAY(LCM_T5);
		lcm_set_gpio_output(LCM_RST_GPIO, GPIO_OUT_ONE);
		MDELAY(LCM_T6);
		init_lcm_registers();
		break;

	case TRS_OFF_TO_ON:
		if (reg_lcm_vcc1v8) {
			ret = regulator_enable(reg_lcm_vcc1v8);
			if (ret != 0)
				LCM_ERR("enable reg_lcm_vcc1v8 fail");
		}
		MDELAY(LCM_T2);
		ret = display_bias_enable_vol(LCM_AVDD_VOL, LCM_AVEE_VOL);
		if (ret < 0)
			LCM_ERR("enable display bias power fail: %d", ret);
		MDELAY(LCM_T3 - LCM_T2 + LCM_T4 + LCM_Tdb);
		lcm_set_gpio_output(TP_RST_GPIO, GPIO_OUT_ONE);
		MDELAY(LCM_T5);
		lcm_set_gpio_output(LCM_RST_GPIO, GPIO_OUT_ONE);
		MDELAY(LCM_T6);
		init_lcm_registers();
		break;

	case TRS_OFF:
		push_table(sleep_in_setting,
			sizeof(sleep_in_setting) / sizeof(struct LCM_setting_table), 1);
		MDELAY(LCM_T10);
		ret = display_bias_disable();
		if (ret < 0)
			LCM_ERR("disable display bias power fail: %d", ret);
		MDELAY(LCM_T11 + LCM_Tdb);
		if (reg_lcm_vcc1v8) {
			ret = regulator_disable(reg_lcm_vcc1v8);
			if (ret != 0)
				LCM_ERR("disable reg_lcm_vcc1v8 fail");
		}
		lcm_set_gpio_output(TP_RST_GPIO, GPIO_OUT_ZERO);
		lcm_set_gpio_output(LCM_RST_GPIO, GPIO_OUT_ZERO);
		break;
	case TRS_KEEP_CUR:
		ret = -1;
		LCM_LOGI("keep current state");
		break;
	case TRS_NOT_PSB:
		ret = -1;
		LCM_ERR("Unexpected state change");
		break;
	default:
		ret = -1;
		LCM_ERR("%s undefined state (%d)", __func__, state);
		break;
	}
	LCM_FUNC_DBG_EXIT();
	return ret;
}

static void lcm_power_manager_lock(int pwr_state)
{
	LCM_FUNC_DBG_ENTRY();
	mutex_lock(&ft8201_share_power_mutex);
	lcm_power_manager(pwr_state);
	mutex_unlock(&ft8201_share_power_mutex);
	LCM_FUNC_DBG_EXIT();
}

static void lcm_power_manager(int new_state)
{
	int tmp_state = LCM_ON;
	int new_trns;
	int ret = 0;

	LCM_FUNC_DBG_ENTRY();

	tmp_state = old_state;

	switch (new_state) {
	case LCM_ON:
	case LCM_ON1:
		atomic_inc(&g_power_request);
		break;
	case LCM_ON2:
	case LCM_SLP:
	case LCM_DSB:
		break;
	case LCM_OFF:
		atomic_dec(&g_power_request);
		if (atomic_read(&g_power_request) <= 0)
			new_state = LCM_OFF;
		else
			new_state = LCM_DSB;
		break;
	}

	new_trns = trans_table[old_state][new_state];

	ret = lcm_power_controller(new_trns);

	if (ret == 0)
		old_state = new_state;

	LCM_LOGI("%s requesting %s, changed from %s to %s, count=%d",
		__func__, lcm_state_str[new_state],
		lcm_state_str[tmp_state], lcm_state_str[old_state],
		atomic_read(&g_power_request));

	LCM_FUNC_DBG_EXIT();
}

static int __init get_lk_parameter(char *str)
{
	int lcm_id;

	if (get_option(&str, &lcm_id)) {
		g_vendor_id = lcm_id;
		LCM_LOGI("lk lcm_id value=%x", lcm_id);
	}
	return 0;
}
__setup("lcm_id=", get_lk_parameter);

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			MDELAY(table[i].count);
			break;

		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

static void lcm_setbacklight_mode(unsigned int mode)
{
	static unsigned last_mode = CABC_OFF;
#ifndef WORKAROUND_FT8201_SETUP_LCM_CABC_THROUGH_TOUCH_I2C
	int ret;
#endif

	LCM_LOGI("%s setup CABC mode=%d\n", __func__, mode);

	if (mode > CABC_MOVIE) {
		LCM_ERR("CABC mode %x is not supported now", mode);
		goto err_unsupport;
	}

	if (mode == last_mode) {
		LCM_LOGI("exit due to no change on cabc mode(%d)", mode);
		goto err_nochange;
	} else {
		last_mode = mode;
	}

	switch (mode) {
	case CABC_MOVIE:
		LCM_DBG("CABC_MOVIE\n");
#ifdef WORKAROUND_FT8201_SETUP_LCM_CABC_THROUGH_TOUCH_I2C
		if (ft8201_share_cabc_write(CABC_ON_VAL) < 0)
			LCM_ERR("CABC write via I2C fail");
#else
		dsi_set_cmdq_V4(cabc_on_setting,
			sizeof(cabc_on_setting)
			/ sizeof(struct LCM_setting_table_V4), 1);
		ret = dsi_get_cmdq_V4(cabc_read,
			sizeof(cabc_read)
			/ sizeof(struct LCM_setting_table_V4), 1);
		LCM_LOGI("CABC_MOVIE: 0x95=0x%x\n", ret);
#endif
		break;

	case CABC_OFF:
		LCM_DBG("CABC_OFF\n");
#ifdef WORKAROUND_FT8201_SETUP_LCM_CABC_THROUGH_TOUCH_I2C
		if (ft8201_share_cabc_write(CABC_OFF_VAL) < 0)
			LCM_ERR("CABC Write via I2C fail");
#else
		dsi_set_cmdq_V4(cabc_off_setting,
			sizeof(cabc_off_setting)
			/ sizeof(struct LCM_setting_table_V4), 1);
		ret = dsi_get_cmdq_V4(cabc_read,
			sizeof(cabc_read)
			/ sizeof(struct LCM_setting_table_V4), 1);
		LCM_LOGI("CABC_MOVIE: 0x95=0x%x\n", ret);
#endif
		break;

	default:
		LCM_ERR("CABC mode %x is not supported now\n", mode);
		break;
	}

err_unsupport:
err_nochange:
	return;
}

LCM_DRIVER ft8201_wuxga_dsi_vdo_lcm_drv = {
	.name = "ft8201_wuxga_dsi_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.init_power = lcm_init_power,
	.suspend_power = lcm_suspend_power,
	.suspend = lcm_suspend,
	.resume_power = lcm_resume_power,
	.resume = lcm_resume,
	.set_backlight_mode = lcm_setbacklight_mode,
#if (LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
};
/* lcm_driver end */
