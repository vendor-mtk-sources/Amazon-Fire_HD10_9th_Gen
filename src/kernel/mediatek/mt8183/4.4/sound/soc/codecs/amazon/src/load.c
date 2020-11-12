/*
 * load.c
 *
 * The ADF FW loading feature with AMZN common ADF format (header).
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
#include "adf/adf_status.h"
#include "adf/adf_common.h"

#define TAG "ADF_LOAD"
#define DSP_CORE_NUM (1)

static adfDspHeader_t dspHeaderArry[DSP_CORE_NUM];

/*
 * adfLoad_getDspHeader()
 * ----------------------------------------
 * return dspHeader info of certain dsp core
 *
 * Input:
 *   int coreNum        - DSP core num
 * Return:
 *   adfDspHeader_t *
 */
adfDspHeader_t *adfLoad_getDspHeader(uint8_t coreNo)
{
	static firstCall;
	int i = 0;

	if (firstCall == 0) {
		for (i = 0; i < DSP_CORE_NUM; i++)
			mutex_init(&(dspHeaderArry[i].dspHeaderLock));
		firstCall = 1;
	}

	if (coreNo >= DSP_CORE_NUM)
		return NULL;
	else
		return &dspHeaderArry[coreNo];
}

/*
 * adfLoad_checkMagic()
 * ----------------------------------------
 * check the FW header whether it is ADF magic.
 * we'll parse the FW header and load bins by ADF process if the magic match
 *
 * Input:
 *   void *data        - the FW header data
 * Return:
 *   true or false
 */
bool adfLoad_checkMagic(const uint8_t *data)
{
	adfFwHdr_t *header = (adfFwHdr_t *)data;

	return (header->magic == ADF_FW_MAGIC);
}

/*
 * adfLoad_init()
 * ----------------------------------------
 * load the adf FW by sections with the info in the header
 * the specified region info, especially SYNC & IPC, will be parsed here, too
 *
 * Input:
 *   void *src                 - the FW binary source buffer
 *   void *dst                 - the base addr (va) of the destination buffer
 *   size_t size               - the length of the FW binary
 *   adfLoad_binCpy binCpy     - bin copy function pointer
 *   adfFwHdr_t **dspHeaderPtr - dspHeader pointer
 * Return:
 *   int32_t, OK or GENERAL_ERROR
 */
int32_t adfLoad_init(void *src, void *dst, uint32_t size,
	adfLoad_binCpy binCpy, uint8_t coreNo)
{
	uint32_t i;
	uint32_t headerSize;
	uint32_t binSize = 0;
	uint8_t *data;
	char platDesc[16] = {0};
	adfFwHdr_t *header = (adfFwHdr_t *)src;
	adfDspHeader_t *adfDspHeader = NULL;
	adfFwHdr_secInfo_t *info;
	uintptr_t baseAddr = 0;
	uint8_t *hostDst = (uint8_t *)dst;
	uint8_t *dstAddr = NULL;
	uint8_t *ver;



	/* dump the basic info and get the headerSize */
	memcpy(platDesc, header->chip, sizeof(platDesc) - 1);
	ver = (uint8_t *)header->version;
	ADF_LOG_I(TAG, "DSP binary: version %02x%02x/%02x/%02x-" \
			  "%02x:%02x:%02x-%02x, platform %s\n",
			  ver[0], ver[1], ver[2], ver[3], ver[4], ver[5], ver[6], ver[7],
			  platDesc);
	headerSize = ADF_FW_HDRLEN +
				 header->seccnt * sizeof(adfFwHdr_secInfo_t);

	adfDspHeader = adfLoad_getDspHeader(coreNo);
	if (adfDspHeader == NULL) {
		ADF_LOG_I(TAG, "dsp core no should smaller than %d\n", DSP_CORE_NUM);
		return ADF_STATUS_GENERAL_ERROR;
	}
	mutex_lock(&adfDspHeader->dspHeaderLock);
	/* we'll always free dspHeader as the len may be different for each proj */
	if (adfDspHeader->dspHeader != NULL) {
		for (i = 0; i < adfDspHeader->dspHeader->seccnt; i++) {
			if (adfDspHeader->dspHeader->secarray[i].data != NULL)
				kfree(adfDspHeader->dspHeader->secarray[i].data);
		}
		kfree(adfDspHeader->dspHeader);
	}
	adfDspHeader->dspHeader = (adfFwHdr_t *)kmalloc((headerSize + 7) & (~0x7), GFP_KERNEL);
	if (adfDspHeader->dspHeader == NULL) {
		ADF_LOG_E(TAG, "Failed to allocate kernel memory for dspHeader\n");
		goto ERROR;
	}
	memcpy(adfDspHeader->dspHeader, src, headerSize);
	info = &(adfDspHeader->dspHeader->secarray[0]);

	/* note that, there are multiple types of section info */
	for (i = 0; i < adfDspHeader->dspHeader->seccnt; i++, info++) {
		if (strcmp(info->name, "BIN") == 0) {
			data = (uint8_t *)src + info->offset;
			binSize += info->size;
			if (baseAddr == 0)
				baseAddr = info->addr;
			if (hostDst == NULL)
				hostDst = (uint8_t *)(uintptr_t)info->addr;
			dstAddr = (uint8_t *)((uintptr_t)hostDst + (info->addr - baseAddr));

			/* Copy application from ROM/DRAM to TCM/DRAM */
			if (binCpy) {
				if (!binCpy(dstAddr, data, info->size)) {
					ADF_LOG_E(TAG, "Failed load section to %p, len %x, addr %x\n",
					  dstAddr, info->size, info->addr);
					goto ERROR;
				} else {
					ADF_LOG_I(TAG, "Load section to %p, len %x, addr %x\n",
					  dstAddr, info->size, info->addr);
				}
			} else {
				ADF_LOG_E(TAG, "binCpy pointer is invalid!\n");
				goto ERROR;
			}
		} else if (info->name[0] >= 'A' && info->name[0] <= 'Z') {
			if (info->data == NULL) {
				info->data = (uint32_t *)kmalloc(info->size, GFP_KERNEL);
				if (info->data == NULL) {
					ADF_LOG_E(TAG, "Failed to allocate kernel memory for %s\n",
							  info->name);
					goto ERROR;
				}
			}
			ADF_LOG_I(TAG, "reserved section, %s 0x%x (len 0x%x)\n",
					  info->name, info->addr, info->size);
			memset(info->data, 0, info->size);
		}
	}
	if (size != headerSize + binSize) {
		ADF_LOG_E(TAG, "FW size mismatch! 0x%x, 0x%x, 0x%x\n",
				  size, headerSize, binSize);
		goto ERROR;
	}
	mutex_unlock(&adfDspHeader->dspHeaderLock);
	return ADF_STATUS_OK;

ERROR:
	/* free the memory for sta/cli/log */
	if (adfDspHeader->dspHeader != NULL) {
		for (i = 0; i < adfDspHeader->dspHeader->seccnt; i++) {
			if (adfDspHeader->dspHeader->secarray[i].data != NULL)
				kfree(adfDspHeader->dspHeader->secarray[i].data);
		}
		kfree(adfDspHeader->dspHeader);
		adfDspHeader->dspHeader = NULL;
	}
	mutex_unlock(&adfDspHeader->dspHeaderLock);
	return ADF_STATUS_GENERAL_ERROR;
}

/*
 * adfLoad_initCheck()
 * ----------------------------------------
 * check firmware load result
 *
 * Input:
 *   void *src                 - the FW binary source buffer
 *   void *dst                 - the base addr (va) of the destination buffer
 *   adfLoad_binRead binRead   - bin read function pointer
 * Return:
 *   bool
 */
bool adfLoad_initCheck(void *src, void *dst, adfLoad_binRead binRead)
{
	uint32_t i;
	adfFwHdr_t *dspHeader = (adfFwHdr_t *)src;
	adfFwHdr_secInfo_t *info;
	uintptr_t baseAddr = 0;
	uint8_t *hostDst = (uint8_t *)dst;
	uint8_t *binAddr = NULL;
	uint8_t *binBuf = NULL;
	uint8_t *binData;
	bool rv = true;

	info = &(dspHeader->secarray[0]);
	for (i = 0; i < dspHeader->seccnt; i++, info++) {
		if (strcmp(info->name, "BIN") == 0) {
			binData = (uint8_t *)src + info->offset;
			if (baseAddr == 0)
				baseAddr = info->addr;
			if (hostDst == NULL)
				hostDst = (uint8_t *)(uintptr_t)info->addr;
			binAddr = (uint8_t *)((uintptr_t)hostDst + (info->addr - baseAddr));
			binBuf = kmalloc((info->size + 7) & (~0x7), GFP_KERNEL);
			if (binBuf) {
				if (binRead(binAddr, binBuf, info->size)) {
					if (memcmp(binData, binBuf, info->size)) {
						rv = false;
						kfree(binBuf);
						break;
					}
				}
				kfree(binBuf);
			} else {
				rv = false;
				break;
			}
		}
	}
	return rv;
}

/*
 * adspLoad_checkMagic()
 * ----------------------------------------
 * check the FW header whether it is ADSP old magic.
 * we'll parse the FW header and load bins by ADSP process if the magic match
 *
 * Input:
 *   void *data        - the FW header data
 * Return:
 *   true or false
 */
bool adspLoad_checkMagic(const uint8_t *data)
{
	adfFwHdr_old_t *header = (adfFwHdr_old_t *)data;

	return (header->magic == ADSP_FW_MAGIC);
}

/*
 * adspLoad_init()
 * ----------------------------------------
 * load the adf FW by sections with the info in the header
 * the specified region info, especially SYNC & IPC, will be parsed here, too
 *
 * Input:
 *   void *src             - the FW binary source buffer
 *   void *dst             - the base addr (va) of the destination buffer
 *   size_t size           - the length of the FW binary
 *   adfLoad_binCpy binCpy - bin copy function pointer
 * Return:
 *   int32_t, OK or GENERAL_ERROR
 */
int32_t adspLoad_init(void *src, void *dst, uint32_t size,
	adfLoad_binCpy binCpy)
{
	uint32_t i;
	uint32_t headerSize;
	uint32_t binSize = 0;
	uint8_t *data;
	char platDesc[16] = {0};
	adfFwHdr_old_t *header = (adfFwHdr_old_t *)src;
	adfFwHdr_binInfo_t *info = &header->binarray[0];
	uintptr_t baseAddr = 0;
	uint8_t *hostDst = (uint8_t *)dst;
	uint8_t *dstAddr = NULL;
	uint8_t *ver;

	memcpy(platDesc, header->chip, sizeof(platDesc) - 1);
	ver = (uint8_t *)header->version;
	ADF_LOG_I(TAG, "DSP binary: version %02x%02x/%02x/%02x-" \
			  "%02x:%02x:%02x-%02x, platform %s\n",
			  ver[0], ver[1], ver[2], ver[3], ver[4], ver[5], ver[6], ver[7],
			  platDesc);
	headerSize = ADSP_FW_HDRLEN +
				 header->bincnt * sizeof(adfFwHdr_binInfo_t);

	for (i = 0; i < header->bincnt; i++) {
		data = (uint8_t *)src + info->offset;
		binSize += info->size;
		if (baseAddr == 0)
			baseAddr = info->addr;
		if (hostDst == NULL)
			hostDst = (uint8_t *)(uintptr_t)info->addr;
		dstAddr = (uint8_t *)((uintptr_t)hostDst + (info->addr - baseAddr));

		/* Copy application from ROM/DRAM to TCM/DRAM */
		if (binCpy) {
			if (!binCpy(dstAddr, data, info->size)) {
				ADF_LOG_E(TAG, "Failed load section to %p, len %x, addr %x\n",
					dstAddr, info->size, info->addr);
				return ADF_STATUS_GENERAL_ERROR;
			} else {
				ADF_LOG_I(TAG, "Load section to %p, len %x, addr %x\n",
					dstAddr, info->size, info->addr);
			}
		} else {
			ADF_LOG_E(TAG, "binCpy pointer is invalid!\n");
			return ADF_STATUS_GENERAL_ERROR;
		}
		info++;
	}
	if (size != headerSize + binSize) {
		ADF_LOG_E(TAG, "FW size mismatch! 0x%x, 0x%x, 0x%x\n",
				  size, headerSize, binSize);
		return ADF_STATUS_GENERAL_ERROR;
	}

	return ADF_STATUS_OK;
}
