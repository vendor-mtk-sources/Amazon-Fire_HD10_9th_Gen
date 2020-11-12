/*
 * log.c
 *
 * log management for all kinds of modules on all DSP platforms
 *
 * Copyright 2019-2020 Amazon Technologies, Inc. All Rights Reserved.
 *
 * Gerard Mu (mqiao@amazon.com)
 * Zengjin Zhao (zzengjin@amazon.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "adf/adf_status.h"
#include "adf/adf_common.h"

#define TAG "ADF_LOG"

DEFINE_MUTEX(adfLogLock);

/*
 * adfLog_dump()
 * ----------------------------------------
 * Dump log to specific buffer
 *
 * Input:
 *   void *logAddr      - address of log buffer
 *   void *dumpBuf      - address of buffer log will dump to
 *   int32_t dumpBufLen - size of buffer log will dump to
 * Return:
 *   int32_t            - size of log dump to buffer
 */
int32_t adfLog_dump(void *logAddr, void *dumpBuf,
	int32_t dumpBufLen, adfLogDumpMode_t dumpMode)
{
	static int flushLen;
	static adfRingbuf_t *rbuf;

	int count = 0;
	int32_t remain = 0;
	int32_t maxDataLen = 0;
	int32_t remainInfoLen = 64;

	/* keep dump one core until remain is zero */
	if (flushLen == 0)
		rbuf = (adfRingbuf_t *) logAddr;
	else {
		if ((uintptr_t) rbuf != (uintptr_t) logAddr) {
			ADF_LOG_I(TAG, "Please keep dump %p until remain is 0\n", rbuf);
			return count;
		}
	}
	if (rbuf) {
		mutex_lock(&adfLogLock);
		maxDataLen = dumpBufLen - remainInfoLen - 1;
		if (dumpMode == ADF_LOG_DUMP_RECENT) {
			if (flushLen) {
				adfRingbuf_reserve(rbuf, flushLen);
				flushLen = 0;
			}
			remain = adfRingbuf_getUsedSize(rbuf);
			if (remain > maxDataLen) {
				flushLen = remain - maxDataLen;
				adfRingbuf_flush(rbuf, flushLen);
			}
			remain = adfRingbuf_getUsedSize(rbuf);
			count = adfRingbuf_read(rbuf, dumpBuf, remain);
			adfRingbuf_reserve(rbuf, flushLen);
			flushLen = 0;
			remain = 0;
		} else {
			remain = adfRingbuf_getUsedSize(rbuf);
			if (remain < maxDataLen)
				count = adfRingbuf_read(rbuf, dumpBuf, remain);
			else
				count = adfRingbuf_read(rbuf, dumpBuf, maxDataLen);
			adfRingbuf_flush(rbuf, count);
			/* reserve when reach end */
			flushLen += count;
			remain = adfRingbuf_getUsedSize(rbuf);
			if (remain == 0) {
				adfRingbuf_reserve(rbuf, flushLen);
				flushLen = 0;
			}
		}
		/* add log dump mode remain info to cat buf */
		count += snprintf(dumpBuf + count, remainInfoLen,
			"\nLog dump mode: %0d\nremain log len : %08x\n", dumpMode, remain);
		if (count >= dumpBufLen - 1)
			count = dumpBufLen - 1;
		mutex_unlock(&adfLogLock);

	}
	return count;
}

/*
 * adfLog_print()
 * ----------------------------------------
 * print all dsp log to kernel msg
 *
 * Input:
 *   void *logAddr      - address of log buffer
 * Return:
 *   none
 */
void adfLog_print(void *logAddr)
{
	int readLen = 0;
	int flushLen = 0;
	char logBuf[1025];
	adfRingbuf_t *rbuf = NULL;
	int i = 0;
	char *pdump = NULL;

	rbuf = (adfRingbuf_t *) logAddr;
	if (rbuf) {
		mutex_lock(&adfLogLock);
		while (1) {
			memset(logBuf, 0, sizeof(logBuf));
			readLen = adfRingbuf_read(rbuf, logBuf, 1024);
			adfRingbuf_flush(rbuf, readLen);
			flushLen += readLen;
			if (readLen > 0) {
				pdump = logBuf;
				for (i = 0; i < readLen; i++) {
					if (logBuf[i] == '\r')
						logBuf[i] = '\0';
					if (logBuf[i] == '\n') {
						logBuf[i] = '\0';
						printk("%s\n", pdump);
						pdump = logBuf + i + 1;
					}
				}
				if (((uintptr_t)pdump) < ((uintptr_t)(logBuf + readLen))) {
					printk("%s\n", pdump);
				}
			} else {
				adfRingbuf_reserve(rbuf, flushLen);
				break;
			}
		}
		mutex_unlock(&adfLogLock);
	}
}
