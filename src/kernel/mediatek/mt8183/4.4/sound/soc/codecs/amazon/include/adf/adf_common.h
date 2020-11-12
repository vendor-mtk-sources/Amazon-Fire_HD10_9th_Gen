/*
 * adf_common.h
 *
 * the common header file for all DSP platforms and host
 *
 * Copyright 2019-2020 Amazon Technologies, Inc. All Rights Reserved.
 *
 * Gerard Mu (mqiao@amazon.com)
 * Zengjin Zhao (zzengjin@amazon.com)
 * TODO: Add additional contributor's names.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef _ADF_COMMON_H_
#define _ADF_COMMON_H_

#include <linux/fs.h>
#include <linux/firmware.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/io.h>

#include "fwheader.h"
#include "ringbuf.h"
#include "state.h"
#include "cli.h"
#include "log.h"
#include "load.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SLEEP_MS(n)                msleep(n)

#define ADF_ALIGN(x, n)            ((x + (n - 1)) & (~(n - 1)))

#ifdef __cplusplus
}
#endif

#endif /* _ADF_COMMON_H_ */
