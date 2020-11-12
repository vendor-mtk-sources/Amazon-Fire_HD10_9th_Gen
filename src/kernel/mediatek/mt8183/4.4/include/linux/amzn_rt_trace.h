/*
 * Copyright (C) 2020 Amazon.com, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __AMZN_RT_TRACE__
#define __AMZN_RT_TRACE__
enum amzn_rt_trace_type {
	AMZN_RT_TRACE_TYPE_DDR = 0,
	AMZN_RT_TRACE_TYPE_CPU = 1,
	AMZN_RT_TRACE_TYPE_MAX = AMZN_RT_TRACE_TYPE_CPU,
};
void amzn_rt_trace(enum amzn_rt_trace_type type,
		   u64 value1, u64 value2);
void amzn_rt_trace_tag(enum amzn_rt_trace_type type, char *tag,
		       u64 value1, u64 value2);
#endif
