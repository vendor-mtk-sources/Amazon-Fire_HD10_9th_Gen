/*
 * KINETIC LMU (Lighting Management Unit) Devices
 *
 * Author: Bruce pu <bruce.xm.pu@enskytech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LEDS_KTZ8864A_H__
#define __LEDS_KTZ8864A_H__

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <uapi/linux/uleds.h>

#define LMU_11BIT_LSB_MASK	(BIT(0) | BIT(1) | BIT(2))
#define LMU_11BIT_MSB_SHIFT	3

#define MAX_BRIGHTNESS_8BIT	255
#define MAX_BRIGHTNESS_11BIT	2047

struct kinetic_lmu_bank {
	struct regmap *regmap;

	int max_brightness;

	u8 lsb_brightness_reg;
	u8 msb_brightness_reg;

	u8 runtime_ramp_reg;
	u32 ramp_up_usec;
	u32 ramp_down_usec;
};

#endif //__LEDS_KTZ8864A_H__
