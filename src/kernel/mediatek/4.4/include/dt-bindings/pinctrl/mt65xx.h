/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Hongzhou.Yang <hongzhou.yang@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _DT_BINDINGS_PINCTRL_MT65XX_H
#define _DT_BINDINGS_PINCTRL_MT65XX_H

#define MTK_PIN_NO(x) ((x) << 8)
#define MTK_GET_PIN_NO(x) ((x) >> 8)
#define MTK_GET_PIN_FUNC(x) ((x) & 0xf)

#define MTK_PUPD_SET_R1R0_00 100
#define MTK_PUPD_SET_R1R0_01 101
#define MTK_PUPD_SET_R1R0_10 102
#define MTK_PUPD_SET_R1R0_11 103
#define MTK_RSEL_SET_R1R0_00 104
#define MTK_RSEL_SET_R1R0_01 105
#define MTK_RSEL_SET_R1R0_10 106
#define MTK_RSEL_SET_R1R0_11 107

/**
 * The mapping here has changed from previous SoCs and is now of the form
 * Drive-strength = (Value + 1) * 2, with a maximum drive strength value of
 * 16 mA, as defined in the mt8183 datasheet.
 * For example, to see a drive-strength of 10mA reflected, we supply a value
 * of four: Drive-strength = (4 + 1) * 2 = 10mA
 */
#define MTK_DRIVE_2mA  0
#define MTK_DRIVE_4mA  1
#define MTK_DRIVE_6mA  2
#define MTK_DRIVE_8mA  3
#define MTK_DRIVE_10mA 4
#define MTK_DRIVE_12mA 5
#define MTK_DRIVE_14mA 6
#define MTK_DRIVE_16mA 7

#define MTK_I2C_EH_DRIVE_000  100
#define MTK_I2C_EH_DRIVE_001  101
#define MTK_I2C_EH_DRIVE_010  102
#define MTK_I2C_EH_DRIVE_011  103
#define MTK_I2C_EH_DRIVE_100  104
#define MTK_I2C_EH_DRIVE_101  105
#define MTK_I2C_EH_DRIVE_110  106
#define MTK_I2C_EH_DRIVE_111  107

#endif /* _DT_BINDINGS_PINCTRL_MT65XX_H */
