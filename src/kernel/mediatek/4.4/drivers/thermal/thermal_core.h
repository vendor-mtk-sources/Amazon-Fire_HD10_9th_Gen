/*
 *  thermal_core.h
 *
 *  Copyright (C) 2012  Intel Corp
 *  Author: Durgadoss R <durgadoss.r@intel.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef __THERMAL_CORE_H__
#define __THERMAL_CORE_H__

#include <linux/device.h>
#include <linux/thermal.h>

/* Initial state of a cooling device during binding */
#define THERMAL_NO_TARGET -1UL

/* 10 seconds default power off delay */
#define THERMAL_DEFAULT_POWEROFF_DELAY_MS (10000)

int thermal_register_governor(struct thermal_governor *);
void thermal_unregister_governor(struct thermal_governor *);

#ifdef CONFIG_THERMAL_GOV_STEP_WISE
int thermal_gov_step_wise_register(void);
void thermal_gov_step_wise_unregister(void);
#else
static inline int thermal_gov_step_wise_register(void) { return 0; }
static inline void thermal_gov_step_wise_unregister(void) {}
#endif /* CONFIG_THERMAL_GOV_STEP_WISE */

#ifdef CONFIG_THERMAL_GOV_FAIR_SHARE
int thermal_gov_fair_share_register(void);
void thermal_gov_fair_share_unregister(void);
#else
static inline int thermal_gov_fair_share_register(void) { return 0; }
static inline void thermal_gov_fair_share_unregister(void) {}
#endif /* CONFIG_THERMAL_GOV_FAIR_SHARE */

#ifdef CONFIG_THERMAL_GOV_BANG_BANG
int thermal_gov_bang_bang_register(void);
void thermal_gov_bang_bang_unregister(void);
#else
static inline int thermal_gov_bang_bang_register(void) { return 0; }
static inline void thermal_gov_bang_bang_unregister(void) {}
#endif /* CONFIG_THERMAL_GOV_BANG_BANG */

#ifdef CONFIG_THERMAL_GOV_USER_SPACE
int thermal_gov_user_space_register(void);
void thermal_gov_user_space_unregister(void);
#else
static inline int thermal_gov_user_space_register(void) { return 0; }
static inline void thermal_gov_user_space_unregister(void) {}
#endif /* CONFIG_THERMAL_GOV_USER_SPACE */

#ifdef CONFIG_THERMAL_GOV_POWER_ALLOCATOR
int thermal_gov_power_allocator_register(void);
void thermal_gov_power_allocator_unregister(void);
#else
static inline int thermal_gov_power_allocator_register(void) { return 0; }
static inline void thermal_gov_power_allocator_unregister(void) {}
#endif /* CONFIG_THERMAL_GOV_POWER_ALLOCATOR */

/* device tree support */
#ifdef CONFIG_THERMAL_OF
int of_parse_thermal_zones(void);
void of_thermal_destroy_zones(void);
int of_thermal_get_ntrips(struct thermal_zone_device *);
void of_thermal_set_ntrips(struct thermal_zone_device *, unsigned int trips);
bool of_thermal_is_trip_valid(struct thermal_zone_device *, int);
const struct thermal_trip *
of_thermal_get_trip_points(struct thermal_zone_device *);
#else
static inline int of_parse_thermal_zones(void) { return 0; }
static inline void of_thermal_destroy_zones(void) { }
static inline int of_thermal_get_ntrips(struct thermal_zone_device *tz)
{
	return 0;
}
static inline void of_thermal_set_ntrips(struct thermal_zone_device *,
		unsigned int trips);
{

}
static inline bool of_thermal_is_trip_valid(struct thermal_zone_device *tz,
					    int trip)
{
	return false;
}
static inline const struct thermal_trip *
of_thermal_get_trip_points(struct thermal_zone_device *tz)
{
	return NULL;
}
#endif

#endif /* __THERMAL_CORE_H__ */
