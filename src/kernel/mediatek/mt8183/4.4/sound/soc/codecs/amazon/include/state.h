/*
 * state.h
 *
 * the state machine management for all DSP platforms
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
#ifndef _ADF_STATE_H_
#define _ADF_STATE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ADF_STATE_DEF  = 0,
	ADF_STATE_INIT = 1,
	ADF_STATE_IDLE = 2,
	ADF_STATE_RUN  = 3,
	ADF_STATE_LPWR = 4,
	ADF_STATE_ERR  = 5,
	ADF_STATE_MAX  = 6
} adfState_t;

/* wakeword magic number */
#define ADF_OPT_WW_MAGIC  0xCAFEBEEF
#define ADF_OPT_NONE      0x00000000

adfState_t adfState_getLast(void *addr);
adfState_t adfState_get(void *addr);
void adfState_set(void *addr, adfState_t state);
uint32_t adfState_getOpt(void *addr);
void adfState_setOpt(void *addr, uint32_t opt);

#ifdef __cplusplus
}
#endif

#endif  /* _ADF_STATE_H_ */
