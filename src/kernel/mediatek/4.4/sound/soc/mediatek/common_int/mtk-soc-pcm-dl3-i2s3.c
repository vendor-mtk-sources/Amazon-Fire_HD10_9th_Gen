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
*
* You should have received a copy of the GNU General Public License
* along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   mt-soc-pcm-dl3-i2s3.c
 *
 * Project:
 * --------
 *    Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio playback for DL3 to I2S3
 *
 * Author:
 * -------
 *
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#include <linux/dma-mapping.h>
#include <sound/pcm_params.h>
#include "mtk-auddrv-common.h"
#include "mtk-soc-pcm-common.h"
#include "mtk-auddrv-def.h"
#include "mtk-auddrv-afe.h"
#include "mtk-auddrv-ana.h"
#include "mtk-auddrv-clk.h"
#include "mtk-auddrv-kernel.h"
#include "mtk-soc-afe-control.h"
#include "mtk-soc-pcm-platform.h"

#ifdef CONFIG_MTK_ACAO_SUPPORT
#include "mtk_mcdi_governor_hint.h"
#endif


#define CLEAR_BUFFER_US         600
static int CLEAR_BUFFER_SIZE;

static struct afe_mem_control_t *pMemControl;
static struct snd_dma_buffer i2s3_dl_dma_buf;
static unsigned int mPlaybackDramState;

static bool vcore_dvfs_enable;


/*
 *    function implementation
 */
static int i2s3_dl_hdoutput;
static bool mPrepareDone;
static struct device *mDev;

static int i2s3_dl_mem_blk;
static int i2s3_dl_mem_blk_io;

const char * const i2s3_dl_HD_output[] = {"Off", "On"};
static const char const *I2S0_I2S3_4pin_switch[] = {"Off", "On"};


static const struct soc_enum Audio_i2s3_dl_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(i2s3_dl_HD_output), i2s3_dl_HD_output),
};

static const struct soc_enum Audio_I2S0_I2S3_4pin_Enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(I2S0_I2S3_4pin_switch), I2S0_I2S3_4pin_switch);


static int Audio_i2s3_dl_hdoutput_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s() = %d\n", __func__, i2s3_dl_hdoutput);
	ucontrol->value.integer.value[0] = i2s3_dl_hdoutput;
	return 0;
}

static int Audio_i2s3_dl_hdoutput_set(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] >
	    ARRAY_SIZE(i2s3_dl_HD_output)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}

	i2s3_dl_hdoutput = ucontrol->value.integer.value[0];

	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_HDMI) == true) {
		pr_debug("return HDMI enabled\n");
		return 0;
	}

	return 0;
}

static int Audio_I2S0_I2S3_4pin_Get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s, I2S0_I2S3_4pin_ctrl = %d\n", __func__,  I2S0_I2S3_4pin_ctrl);
	ucontrol->value.integer.value[0] = I2S0_I2S3_4pin_ctrl;
	return 0;
}

static int Audio_I2S0_I2S3_4pin_Set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s, set %ld\n", __func__, ucontrol->value.integer.value[0]);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(I2S0_I2S3_4pin_switch)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}

	I2S0_I2S3_4pin_ctrl = ucontrol->value.integer.value[0];
	return 0;
}

static const struct snd_kcontrol_new Audio_i2s3_dl_controls[] = {
	SOC_ENUM_EXT("Audio_i2s3_dl_hd_Switch", Audio_i2s3_dl_Enum[0],
		     Audio_i2s3_dl_hdoutput_get, Audio_i2s3_dl_hdoutput_set),
	SOC_ENUM_EXT("Audio_I2S0_I2S3_4pin", Audio_I2S0_I2S3_4pin_Enum,
			Audio_I2S0_I2S3_4pin_Get, Audio_I2S0_I2S3_4pin_Set),
};

static struct snd_pcm_hardware mtk_i2s3_dl_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP |
	SNDRV_PCM_INFO_INTERLEAVED |
	SNDRV_PCM_INFO_RESUME |
	SNDRV_PCM_INFO_MMAP_VALID),
	.formats =   SND_SOC_ADV_MT_FMTS,
	.rates =        SOC_HIGH_USE_RATE,
	.rate_min =     SOC_HIGH_USE_RATE_MIN,
	.rate_max =     SOC_HIGH_USE_RATE_MAX,
	.channels_min =     SOC_NORMAL_USE_CHANNELS_MIN,
	.channels_max =     SOC_NORMAL_USE_CHANNELS_MAX,
	.buffer_bytes_max = SOC_HIFI_DEEP_BUFFER_SIZE,
	.period_bytes_max = SOC_HIFI_DEEP_BUFFER_SIZE,
	.periods_min =      SOC_NORMAL_USE_PERIODS_MIN,
	.periods_max =     SOC_NORMAL_USE_PERIODS_MAX,
	.fifo_size =        0,
};


static int mtk_pcm_i2s3_dl_stop(struct snd_pcm_substream *substream)
{
	/* struct afe_block_t *Afe_Block = &(pMemControl->rBlock); */

	pr_debug("%s\n", __func__);

	irq_remove_user(substream, irq_request_number(i2s3_dl_mem_blk));

	SetMemoryPathEnable(i2s3_dl_mem_blk, false);

	ClearMemBlock(i2s3_dl_mem_blk);

	return 0;
}

static snd_pcm_uframes_t mtk_pcm_i2s3_dl_pointer(struct snd_pcm_substream
						    *substream)
{
	unsigned int ptr_bytes = 0, hw_ptr, base, cur;

	base = GetBufferCtrlReg(i2s3_dl_mem_blk_io, aud_buffer_ctrl_base);
	cur = GetBufferCtrlReg(i2s3_dl_mem_blk_io, aud_buffer_ctrl_cur);

	if (!base || !cur) {
		pr_warn("%s(), GetBufferCtrlReg return invalid value\n", __func__);
	} else {
		hw_ptr = Afe_Get_Reg(cur);
		if (hw_ptr == 0)
			pr_warn("%s(), hw_ptr == 0\n", __func__);
		else
			ptr_bytes = hw_ptr - Afe_Get_Reg(base);

		ptr_bytes = word_size_align(ptr_bytes);
	}

	return bytes_to_frames(substream->runtime, ptr_bytes);
}

static int mtk_pcm_i2s3_dl_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *hw_params)
{
	int ret = 0;

	substream->runtime->dma_bytes = params_buffer_bytes(hw_params);
	if (substream->runtime->dma_bytes <= GetPLaybackSramFullSize() &&
	    AllocateAudioSram(&substream->runtime->dma_addr,
			      &substream->runtime->dma_area,
			      substream->runtime->dma_bytes,
			      substream,
			      params_format(hw_params), false) == 0) {
		SetHighAddr(i2s3_dl_mem_blk, false, substream->runtime->dma_addr);
	} else {
		substream->runtime->dma_area = i2s3_dl_dma_buf.area;
		substream->runtime->dma_addr = i2s3_dl_dma_buf.addr;
		SetHighAddr(i2s3_dl_mem_blk, true, substream->runtime->dma_addr);
		mPlaybackDramState = true;
		AudDrv_Emi_Clk_On();
	}

	set_mem_block(substream, hw_params, pMemControl,
		      i2s3_dl_mem_blk);

	pr_debug("dma_bytes = %zu dma_area = %p dma_addr = 0x%lx\n",
	       substream->runtime->dma_bytes, substream->runtime->dma_area,
	       (long)substream->runtime->dma_addr);

	return ret;
}

static int mtk_pcm_i2s3_dl_hw_free(struct snd_pcm_substream *substream)
{
	pr_debug("%s substream = %p\n", __func__, substream);
	if (mPlaybackDramState == true) {
		AudDrv_Emi_Clk_Off();
		mPlaybackDramState = false;
	} else
		freeAudioSram((void *)substream);

	return 0;
}

static struct snd_pcm_hw_constraint_list constraints_sample_rates = {
	.count = ARRAY_SIZE(soc_high_supported_sample_rates),
	.list = soc_high_supported_sample_rates,
	.mask = 0,
};

static int mtk_pcm_i2s3_dl_close(struct snd_pcm_substream *substream)
{
	pr_debug("%s\n", __func__);

	if (is_irq_from_ext_module()) {
		ext_sync_signal_lock();
		ext_sync_signal_unlock();
	}

	if (mPrepareDone == true) {
		SetIntfConnection(Soc_Aud_InterCon_DisConnect, i2s3_dl_mem_blk_io,
				  Soc_Aud_AFE_IO_Block_I2S3);

		if (I2S0_I2S3_4pin_ctrl) {
			Disable4pin_I2S0_I2S3();
		} else {
			SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_2, false);
			if (GetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_2) == false)
				Afe_Set_Reg(AFE_I2S_CON3, 0x0, 0x1);
		}

		RemoveMemifSubStream(i2s3_dl_mem_blk, substream);

		if (i2s3_dl_hdoutput == true) {
			pr_debug("%s i2s3_dl_hdoutput == %d\n", __func__,
				 i2s3_dl_hdoutput);

			/* here to close APLL */
			if (!mtk_soc_always_hd) {
				DisableAPLLTunerbySampleRate(substream->runtime->rate);
				DisableALLbySampleRate(substream->runtime->rate);
			}

			//EnableI2SCLKDiv(Soc_Aud_I2S1_MCKDIV, false);
		}
		EnableAfe(false);
		mPrepareDone = false;
	}

	AudDrv_Clk_Off();

	vcore_dvfs(&vcore_dvfs_enable, true);

#ifdef CONFIG_MTK_ACAO_SUPPORT
	system_idle_hint_request(SYSTEM_IDLE_HINT_USER_AUDIO, 0);
#endif

	return 0;
}

static int mtk_pcm_i2s3_dl_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;

	pr_debug("%s: hardware.buffer_bytes_max = %zu mPlaybackDramState = %d\n",
		 __func__, mtk_i2s3_dl_hardware.buffer_bytes_max, mPlaybackDramState);

	mPlaybackDramState = false;
	runtime->hw = mtk_i2s3_dl_hardware;
	memcpy((void *)(&(runtime->hw)), (void *)&mtk_i2s3_dl_hardware,
	       sizeof(struct snd_pcm_hardware));

	AudDrv_Clk_On();

#ifdef CONFIG_MTK_ACAO_SUPPORT
	system_idle_hint_request(SYSTEM_IDLE_HINT_USER_AUDIO, 1);
#endif

	pMemControl = Get_Mem_ControlT(i2s3_dl_mem_blk);

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &constraints_sample_rates);

	if (ret < 0) {
		pr_err("snd_pcm_hw_constraint_integer failed, close pcm\n");
		mtk_pcm_i2s3_dl_close(substream);
		return ret;
	}

	return 0;
}

static int mtk_pcm_i2s3_dl_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	bool mI2SWLen;
	unsigned int u32AudioI2S = 0;

	pr_debug("%s: mPrepareDone = %d, format = %d, SNDRV_PCM_FORMAT_S32_LE = %d, SNDRV_PCM_FORMAT_U32_LE = %d, sample rate = %d\n",
			       __func__,
			       mPrepareDone,
			       runtime->format,
			       SNDRV_PCM_FORMAT_S32_LE, SNDRV_PCM_FORMAT_U32_LE,
			       substream->runtime->rate);

	if (mPrepareDone == false) {
		SetMemifSubStream(i2s3_dl_mem_blk, substream);

		if (runtime->format == SNDRV_PCM_FORMAT_S32_LE ||
		    runtime->format == SNDRV_PCM_FORMAT_U32_LE) {
			SetMemIfFetchFormatPerSample(i2s3_dl_mem_blk,
						     AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA);
			SetConnectionFormat(OUTPUT_DATA_FORMAT_24BIT, Soc_Aud_AFE_IO_Block_I2S3);
			mI2SWLen = Soc_Aud_I2S_WLEN_WLEN_32BITS;
		} else {
			SetMemIfFetchFormatPerSample(i2s3_dl_mem_blk, AFE_WLEN_16_BIT);
			SetConnectionFormat(OUTPUT_DATA_FORMAT_16BIT, Soc_Aud_AFE_IO_Block_I2S3);
			mI2SWLen = Soc_Aud_I2S_WLEN_WLEN_16BITS;
		}

		SetIntfConnection(Soc_Aud_InterCon_Connection, i2s3_dl_mem_blk_io,
				  Soc_Aud_AFE_IO_Block_I2S3);

		/* I2S out Setting */
		if (i2s3_dl_hdoutput == true) {
			pr_debug("%s deep_buffer_dl_hdoutput == %d\n", __func__,
				 i2s3_dl_hdoutput);
			/* here to open APLL */
			if (!mtk_soc_always_hd) {
				EnableALLbySampleRate(runtime->rate);
				EnableAPLLTunerbySampleRate(runtime->rate);
			}

			//SetCLkMclk(Soc_Aud_I2S1, runtime->rate); /* select I2S */
			//EnableI2SCLKDiv(Soc_Aud_I2S1_MCKDIV, true);
		}

		if (I2S0_I2S3_4pin_ctrl) {
			Enable4pin_I2S0_I2S3(substream->runtime->rate, i2s3_dl_hdoutput, mI2SWLen);
		} else {
			u32AudioI2S = SampleRateTransform(runtime->rate, Soc_Aud_Digital_Block_I2S_OUT_2) << 8;
			u32AudioI2S |= Soc_Aud_I2S_FORMAT_I2S << 3; /* use I2s format */
			u32AudioI2S |= mI2SWLen << 1;
			u32AudioI2S |= (i2s3_dl_hdoutput ? Soc_Aud_LOW_JITTER_CLOCK : Soc_Aud_NORMAL_CLOCK) << 12;

			if (GetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_2) == false) {
				SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_2, true);
				Afe_Set_Reg(AFE_I2S_CON3, u32AudioI2S | 1, AFE_MASK_ALL);
			} else {
				SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_2, true);
			}

		}

		EnableAfe(true);
		mPrepareDone = true;

		CLEAR_BUFFER_SIZE = substream->runtime->rate * CLEAR_BUFFER_US *
				    audio_frame_to_bytes(substream, 1) / 1000000;
	}
	return 0;
}

static int mtk_pcm_i2s3_dl_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	pr_debug("%s\n", __func__);

	/* here to set interrupt */
	irq_add_user(substream,
		     irq_request_number(i2s3_dl_mem_blk),
		     substream->runtime->rate,
		     substream->runtime->period_size);

	SetSampleRate(i2s3_dl_mem_blk, runtime->rate);
	SetChannels(i2s3_dl_mem_blk, runtime->channels);
	SetMemoryPathEnable(i2s3_dl_mem_blk, true);

	EnableAfe(true);

	return 0;
}

static int mtk_pcm_i2s3_dl_trigger(struct snd_pcm_substream *substream, int cmd)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		return mtk_pcm_i2s3_dl_start(substream);
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return mtk_pcm_i2s3_dl_stop(substream);
	}
	return -EINVAL;
}

static void *dummy_page[2];
static struct page *mtk_pcm_i2s3_dl_page(struct snd_pcm_substream *substream,
				       unsigned long offset)
{
	pr_debug("%s\n", __func__);
	return virt_to_page(dummy_page[substream->stream]); /* the same page */
}

static int mtk_pcm_i2s3_dl_ack(struct snd_pcm_substream *substream)
{
	int size_per_frame = audio_frame_to_bytes(substream, 1);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int copy_size = word_size_align(snd_pcm_playback_avail(runtime)*size_per_frame);

	if (pMemControl == NULL || mPrepareDone == false)
		return 0;

	if (copy_size > CLEAR_BUFFER_SIZE)
		copy_size = CLEAR_BUFFER_SIZE;

	if (copy_size > 0) {
		struct afe_block_t *afe_block_t = &pMemControl->rBlock;
		snd_pcm_uframes_t appl_ofs = runtime->control->appl_ptr % runtime->buffer_size;
		int32_t u4WriteIdx = appl_ofs * size_per_frame;

		if (u4WriteIdx + copy_size < afe_block_t->u4BufferSize) {
			memset_io(afe_block_t->pucVirtBufAddr + u4WriteIdx, 0, copy_size);
			/*
			* pr_debug("%s A, offset %d, clear buffer %d, copy_size %d\n",
			*          __func__, u4WriteIdx, copy_size, copy_size);
			*/
		} else {
			int32_t size_1 = 0, size_2 = 0;

			size_1 = word_size_align((afe_block_t->u4BufferSize - u4WriteIdx));
			size_2 = word_size_align((copy_size - size_1));

			if (size_1 < 0 || size_2 < 0) {
				pr_debug("%s, copy size error!!\n", __func__);
				pr_debug("u4BufferSize %d, u4WriteIdx %d\n", afe_block_t->u4BufferSize, u4WriteIdx);
				return 0;
			}

			memset_io(afe_block_t->pucVirtBufAddr + u4WriteIdx, 0, size_1);
			memset_io(afe_block_t->pucVirtBufAddr, 0, size_2);
			/*
			 * pr_debug("%s B-1, offset %d, clear buffer %d, copy_size %d\n",
			 *          __func__, u4WriteIdx, size_1, copy_size);
			 * pr_debug("%s B-2, offset %d, clear buffer %d, copy_size %d\n",
			 *          __func__, 0, size_2, copy_size);
			 */
		}
	}

	return 0;
}

static struct snd_pcm_ops mtk_i2s3_dl_ops = {
	.open =     mtk_pcm_i2s3_dl_open,
	.close =    mtk_pcm_i2s3_dl_close,
	.ioctl =    snd_pcm_lib_ioctl,
	.hw_params = mtk_pcm_i2s3_dl_hw_params,
	.hw_free =  mtk_pcm_i2s3_dl_hw_free,
	.prepare =  mtk_pcm_i2s3_dl_prepare,
	.trigger =  mtk_pcm_i2s3_dl_trigger,
	.pointer =  mtk_pcm_i2s3_dl_pointer,
	.page =     mtk_pcm_i2s3_dl_page,
	.ack =      mtk_pcm_i2s3_dl_ack,
};

static int mtk_i2s3_dl_platform_probe(struct snd_soc_platform *platform)
{
	i2s3_dl_mem_blk = Soc_Aud_Digital_Block_MEM_DL3;
	i2s3_dl_mem_blk_io = Soc_Aud_AFE_IO_Block_MEM_DL3;

	pr_debug("%s(), i2s3_dl_mem_blk %d, i2s3_dl_mem_blk_io %d\n",
		 __func__, i2s3_dl_mem_blk, i2s3_dl_mem_blk_io);

	snd_soc_add_platform_controls(platform, Audio_i2s3_dl_controls,
				      ARRAY_SIZE(Audio_i2s3_dl_controls));
	/* allocate dram */
	i2s3_dl_dma_buf.area = dma_alloc_coherent(platform->dev,
						SOC_HIFI_DEEP_BUFFER_SIZE,
						&i2s3_dl_dma_buf.addr,
						GFP_KERNEL | GFP_DMA);
	if (!i2s3_dl_dma_buf.area)
		return -ENOMEM;

	i2s3_dl_dma_buf.bytes = SOC_HIFI_DEEP_BUFFER_SIZE;
	pr_debug("area = %p\n", i2s3_dl_dma_buf.area);

	return 0;
}

static struct snd_soc_platform_driver mtk_i2s3_dl_soc_platform = {
	.ops        = &mtk_i2s3_dl_ops,
	.probe      = mtk_i2s3_dl_platform_probe,
};

static int mtk_i2s3_dl_probe(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	if (pdev->dev.of_node)
		dev_set_name(&pdev->dev, "%s", MT_SOC_I2S3_DL3_PCM);

	pr_debug("%s: dev name %s\n", __func__, dev_name(&pdev->dev));

	mDev = &pdev->dev;

	return snd_soc_register_platform(&pdev->dev,
					 &mtk_i2s3_dl_soc_platform);
}

static int mtk_i2s3_dl_remove(struct platform_device *pdev)
{
	pr_debug("%s()\n", __func__);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_soc_pcm_i2s3_dl_of_ids[] = {
	{ .compatible = "mediatek,mt_soc_pcm_i2s3_dl3", },
	{}
};
#endif

static struct platform_driver mtk_i2s3_dl_driver = {
	.driver = {
		.name = MT_SOC_I2S3_DL3_PCM,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mt_soc_pcm_i2s3_dl_of_ids,
#endif
	},
	.probe = mtk_i2s3_dl_probe,
	.remove = mtk_i2s3_dl_remove,
};

module_platform_driver(mtk_i2s3_dl_driver);

MODULE_DESCRIPTION("AFE PCM module platform driver");
MODULE_LICENSE("GPL");
