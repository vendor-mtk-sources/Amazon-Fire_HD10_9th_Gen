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
 *   mt_soc_pcm_I2S0dl1.c
 *
 * Project:
 * --------
 *    Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio I2S0dl1 and Dl1 playback
 *
 * Author:
 * -------
 * Chipeng Chang
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

#define DEFAULT_PLAYBACK_STATE 0

static struct afe_mem_control_t *pI2S0dl1MemControl;
static struct snd_dma_buffer Dl1I2S0_Playback_dma_buf;
static unsigned int mPlaybackDramState;
static bool vcore_dvfs_enable;
extern int mt_soc_i2s1_clk_always_on(void);
extern int mt_soc_i2s2_clk_always_on(void);
extern int mt_aud_i2s1_bit_width(void);
extern int mtk_I2S0dl1_clock_on(void);
extern int mt_aud_i2s1_samplerate(void);
extern int mt_aud_i2s2_samplerate(void);

struct mt_pcm_i2s0Dl1_priv {
	int playback_state;
};

/*
 *    function implementation
 */
static long long mtk_pcm_I2S0dl1_get_next_write_timestamp(void);
static int mtk_I2S0dl1_probe(struct platform_device *pdev);
static int mtk_pcm_I2S0dl1_close(struct snd_pcm_substream *substream);
static int mtk_afe_I2S0dl1_probe(struct snd_soc_platform *platform);

static int mI2S0dl1_hdoutput_control = true;
static bool mPrepareDone;
static bool mCopyFlag;

static const void *irq_user_id;
static unsigned int irq1_cnt;

static struct device *mDev;
static struct snd_pcm_substream *I2S0dl1_substream;
extern u64 mtk_timer_get_cnt(u8 timer);
static unsigned long copy_count;
#define GPT6_CLOCK_COUNT_PER_SEC (13000000LL) /* 13MHz for GPT6 */
#define BYTES_OF_INT64 (sizeof(int64_t))

const char * const I2S0dl1_HD_output[] = {"Off", "On"};
bool mI21SWLen;

static const struct soc_enum Audio_I2S0dl1_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(I2S0dl1_HD_output), I2S0dl1_HD_output),
};

static int Audio_I2S0dl1_timestamp_Get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	/* calculate the remain buffer for the timestamp */
	long long mI2S0dl1_next_write_timestamp;

	mI2S0dl1_next_write_timestamp = mtk_pcm_I2S0dl1_get_next_write_timestamp();
	ucontrol->value.integer64.value[0] = mI2S0dl1_next_write_timestamp;
	pr_debug("Audio_I2S0dl1_timestamp_Get = 0x%llx\n", mI2S0dl1_next_write_timestamp);
	return 0;
}

static int Audio_I2S0dl1_timestamp_Set(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("Audio_I2S0dl1_timestamp_Set do nothing\n");
	return 0;
}

/* always on i2s1 and i2s2 mck and bck at boot-up */
int mtk_I2S0dl1_clock_on(void)
{
	pr_warn("%s\n", __func__);

	if ((mt_soc_i2s1_clk_always_on()) || (mt_soc_i2s2_clk_always_on())) {

		AudDrv_ANA_Clk_On();
		AudDrv_Clk_On();
		EnableALLbySampleRate(mt_aud_i2s1_samplerate());
		EnableAPLLTunerbySampleRate(mt_aud_i2s1_samplerate());

		/* always on i2s1 mck and bck */
		if (mt_soc_i2s1_clk_always_on()) {
			SetCLkMclk(Soc_Aud_I2S1, mt_aud_i2s1_samplerate());
			EnableI2SCLKDiv(Soc_Aud_I2S1_MCKDIV, true);
			SetSampleRate(Soc_Aud_Digital_Block_MEM_I2S, mt_aud_i2s1_samplerate());
			SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, true);
			SetI2SDacOut(mt_aud_i2s1_samplerate(),1, mI21SWLen);
			SetI2SDacEnable(true);
		}
		/* always on i2s2 bck */
		if (mt_soc_i2s2_clk_always_on()) {
			SetCLkMclk(Soc_Aud_I2S2, mt_aud_i2s2_samplerate());
			EnableI2SCLKDiv(Soc_Aud_I2S2_MCKDIV, true);
			/* SetSampleRate(Soc_Aud_Digital_Block_I2S_IN, 16000); */
			SetI2s2Adc2Out(mt_aud_i2s2_samplerate());
		}
	}

	return 0;
}
EXPORT_SYMBOL(mtk_I2S0dl1_clock_on);

int mtk_I2S0dl1_clock_off(void)
{
	pr_warn("%s\n", __func__);
	if ((!mt_soc_i2s1_clk_always_on()) && (!mt_soc_i2s2_clk_always_on())) {
		AudDrv_ANA_Clk_Off();
		AudDrv_Clk_Off();
		DisableALLbySampleRate(48000);
		DisableAPLLTunerbySampleRate(48000);
	}

	if (!mt_soc_i2s1_clk_always_on()) {
		if (GetI2SDacEnable() == false)
			SetI2SDacEnable(false); /* disable for alway on I2S1 bck/mck */
		SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, false);
	}

	if (!mt_soc_i2s2_clk_always_on()) {
		Afe_Set_Reg(AFE_I2S_CON2, 0x0, 0x1); /* disable for alway on I2S2 bck/mck */
		SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_IN, false);
	}
	/* EnableAfe(false); */

	return 0;
}
EXPORT_SYMBOL(mtk_I2S0dl1_clock_off);

static int Audio_I2S0dl1_hdoutput_Get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("Audio_AmpR_Get = %d\n", mI2S0dl1_hdoutput_control);
	ucontrol->value.integer.value[0] = mI2S0dl1_hdoutput_control;
	return 0;
}

static int Audio_I2S0dl1_hdoutput_Set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	/* pr_debug("%s()\n", __func__); */
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(I2S0dl1_HD_output)) {
		pr_warn("return -EINVAL\n");
		return -EINVAL;
	}

	mI2S0dl1_hdoutput_control = ucontrol->value.integer.value[0];

	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_HDMI) == true) {
		pr_err("return HDMI enabled\n");
		return 0;
	}

	return 0;
}

static int Audio_Irqcnt1_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	AudDrv_Clk_On();
	ucontrol->value.integer.value[0] = Afe_Get_Reg(AFE_IRQ_MCU_CNT1);
	AudDrv_Clk_Off();
	return 0;
}

static int Audio_Irqcnt1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s(), irq_user_id = %p, irq1_cnt = %d, value = %ld\n",
		__func__,
		irq_user_id,
		irq1_cnt,
		ucontrol->value.integer.value[0]);

	if (irq1_cnt == ucontrol->value.integer.value[0])
		return 0;

	irq1_cnt = ucontrol->value.integer.value[0];

	AudDrv_Clk_On();
	if (irq_user_id && irq1_cnt)
		irq_update_user(irq_user_id,
				Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE,
				0,
				irq1_cnt);
	else
		pr_warn("warn, cannot update irq counter, user_id = %p, irq1_cnt = %d\n",
			irq_user_id, irq1_cnt);

	AudDrv_Clk_Off();
	return 0;
}

static int Audio_I2S0dl1_playback_status_Set(struct snd_kcontrol *kcontrol,
                                           struct snd_ctl_elem_value *ucontrol)
{
	struct mt_pcm_i2s0Dl1_priv *priv = dev_get_drvdata(mDev);
	priv->playback_state = ucontrol->value.integer.value[0];
	pr_debug("Audio_I2S0dl1_playback_status_Set = %d\n", priv->playback_state);
	return 0;
}

static int Audio_I2S0dl1_playback_status_Get(struct snd_kcontrol *kcontrol,
                                           struct snd_ctl_elem_value *ucontrol)
{
	struct mt_pcm_i2s0Dl1_priv *priv = dev_get_drvdata(mDev);
	ucontrol->value.integer.value[0] = priv->playback_state;
	pr_debug("Audio_I2S0dl1_playback_status_Get = %d\n", priv->playback_state);
	return 0;
}

static const struct snd_kcontrol_new Audio_snd_I2S0dl1_controls[] = {
	SOC_ENUM_EXT("Audio_I2S0dl1_hd_Switch", Audio_I2S0dl1_Enum[0],
		Audio_I2S0dl1_hdoutput_Get, Audio_I2S0dl1_hdoutput_Set),
	SOC_SINGLE_EXT("Audio IRQ1 CNT", SND_SOC_NOPM, 0, IRQ_MAX_RATE, 0,
		Audio_Irqcnt1_Get, Audio_Irqcnt1_Set),
	SOC_SINGLE_BOOL_EXT("Audio_I2S0dl1_playback_state",
		DEFAULT_PLAYBACK_STATE,
		Audio_I2S0dl1_playback_status_Get,
		Audio_I2S0dl1_playback_status_Set),
	SND_SOC_BYTES_EXT("Audio_I2S0dl1_get_timestamp",
		BYTES_OF_INT64, Audio_I2S0dl1_timestamp_Get,
		Audio_I2S0dl1_timestamp_Set)
};

static struct snd_pcm_hardware mtk_I2S0dl1_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_NO_PERIOD_WAKEUP |
		 SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_RESUME | SNDRV_PCM_INFO_MMAP_VALID),
	.formats =   SND_SOC_ADV_MT_FMTS,
	.rates =        SOC_HIGH_USE_RATE,
	.rate_min =     SOC_HIGH_USE_RATE_MIN,
	.rate_max =     SOC_HIGH_USE_RATE_MAX,
	.channels_min =     SOC_NORMAL_USE_CHANNELS_MIN,
	.channels_max =     SOC_NORMAL_USE_CHANNELS_MAX,
	.buffer_bytes_max = SOC_HIFI_BUFFER_SIZE,
	.period_bytes_max = SOC_HIFI_BUFFER_SIZE,
	.periods_min =      SOC_NORMAL_USE_PERIODS_MIN,
	.periods_max =     SOC_NORMAL_USE_PERIODS_MAX,
	.fifo_size =        0,
};

static long long mtk_pcm_I2S0dl1_get_next_write_timestamp(void)
{
	kal_int32 HW_memory_index = 0;
	kal_int32 HW_Cur_ReadIdx = 0;
	kal_int32 Afe_consumed_bytes = 0;
	kal_int32 u4DataRemained = 0;
	struct afe_block_t *Afe_Block = NULL;
	unsigned long flags = 0 ;
	long long timestamp = 0;
	long long timeForDataRemained = 0;
	unsigned int u4AdjustDataRemainedSample = 0;
	long tmp = 0;
	int rate = 0;
	kal_int32 ValMcuIrq1Mon = 0;

	if (pI2S0dl1MemControl == NULL) {
		return -1;
	} else {
		Afe_Block = &pI2S0dl1MemControl->rBlock;
		if (Afe_Block == NULL)
			return -1;
	}

	spin_lock_irqsave(&pI2S0dl1MemControl->substream_lock, flags);
	PRINTK_AUD_DL1("%s Afe_Block->u4DMAReadIdx = 0x%x\n", __func__,
		Afe_Block->u4DMAReadIdx);

	if ((GetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL1) == true)
					&& (I2S0dl1_substream != NULL)) {
		HW_Cur_ReadIdx = Afe_Get_Reg(AFE_DL1_CUR);
		ValMcuIrq1Mon = Afe_Get_Reg(AFE_IRQ1_MCU_CNT_MON);
		timestamp = (long long)mtk_timer_get_cnt(6);

		if (HW_Cur_ReadIdx == 0) {
			PRINTK_AUDDRV("[Auddrv] HW_Cur_ReadIdx == 0\n");
			HW_Cur_ReadIdx = Afe_Block->pucPhysBufAddr;
		}

		HW_memory_index = (HW_Cur_ReadIdx - Afe_Block->pucPhysBufAddr);

		if (HW_memory_index >= Afe_Block->u4DMAReadIdx)
			Afe_consumed_bytes = HW_memory_index - Afe_Block->u4DMAReadIdx;
		else {
			Afe_consumed_bytes = Afe_Block->u4BufferSize + HW_memory_index
				- Afe_Block->u4DMAReadIdx;
		}
		Afe_consumed_bytes = word_size_align(Afe_consumed_bytes);

		u4DataRemained = Afe_Block->u4DataRemained - Afe_consumed_bytes;
		spin_unlock_irqrestore(&pI2S0dl1MemControl->substream_lock, flags);

		tmp = audio_bytes_to_frame(I2S0dl1_substream, u4DataRemained);
		tmp = tmp - ValMcuIrq1Mon;
		/* Interrupt period = 1/4 kernel buffer. */
		/* ValMcuIrq1Mon : real remaided samples in current half buffer. */
		/* u4DataRemained>>2(16tbit, 2-channels): Remained samples
		  count by current pointer(still some samples in prefetch buffer)*/
		/* if tmp < 0 , it means no extra data except this half buffer */
		/* else there is still data and we can count  */
		/*  <-copy_count->               |<------------IRQ------------>| */
		/* |--------------|--------------|--------------|--------------| */
		/*                                   |<--  ValMcuIrq1Mon  ---->| */
		/*                                                               */
		/*CASE1 : There is no data in another half buffer                */
		/* |                             |   |<--  ValMcuIrq1Mon  ---->| */
		/*                                                               */
		/*CASE2 :                                                        */
		/*   There is 1/4buffer(copy_count)data in another half buffer   */
		/* |XXcopy_countXX|              |   |<--  ValMcuIrq1Mon  ---->| */
		/*                                                               */
		/*CASE3 :                                                        */
		/*  There is 1/4buffer(2*copy_count)data in another half buffer  */
		/* |XXcopy_countXX|XXcopy_countXX|   |<--  ValMcuIrq1Mon  ---->| */
		/*                                                               */
		u4AdjustDataRemainedSample = ValMcuIrq1Mon;
		if (tmp >= 0) {
			/*u4DataRemained larger than irq monitor, it mean there are*/
			/*extra data , and need to add extra data                   */
			while (tmp > 0) {
				u4AdjustDataRemainedSample += copy_count;
				tmp -= copy_count;
			}
		} else {
			/*u4DataRemained less than irq monitor, check if the delta is */
			/* only pre-fetch buffer(192 bytes), if not, need to calculate*/
			/* real data by copy_count & the difference                   */
			tmp = tmp + audio_bytes_to_frame(I2S0dl1_substream, 256) ;
			while (tmp <= 0) {
				if (u4AdjustDataRemainedSample < copy_count) {
					u4AdjustDataRemainedSample = 0;
					pr_err("%s avoid underflow ", __func__);
					pr_err("set u4AdjustDataRemainedSample =0\n");
					break;
				}
				u4AdjustDataRemainedSample -= copy_count;
				tmp += copy_count;
			}
		}

		rate = (int) I2S0dl1_substream->runtime->rate;

		timeForDataRemained = GPT6_CLOCK_COUNT_PER_SEC * (long long)u4AdjustDataRemainedSample;
		do_div(timeForDataRemained, (long long)rate);

		PRINTK_AUD_DL1(
			"mon=%d,rem=%d,adj=%d,timeRem=%lld,ts0=%lld,ts=%lld\n",
			ValMcuIrq1Mon, u4DataRemained,
			u4AdjustDataRemainedSample, timeForDataRemained,
			timestamp, timestamp + timeForDataRemained);

		timestamp += timeForDataRemained;
		return timestamp;
	}

	/* should not happen. */
	pr_err("%s: MEM path to DL1 isn't enable, get timestamp=-1", __func__);
	spin_unlock_irqrestore(&pI2S0dl1MemControl->substream_lock, flags);
	return 0x7FFFFFFFFFFFFFFFLL;
}

static int mtk_pcm_I2S0dl1_stop(struct snd_pcm_substream *substream)
{
	/* struct afe_block_t *Afe_Block = &(pI2S0dl1MemControl->rBlock); */
	struct mt_pcm_i2s0Dl1_priv *priv = dev_get_drvdata(mDev);
	pr_warn("%s\n", __func__);

	irq_user_id = NULL;
	irq_remove_substream_user(substream, irq_request_number(Soc_Aud_Digital_Block_MEM_DL1));

	SetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL1, false);

	ClearMemBlock(Soc_Aud_Digital_Block_MEM_DL1);
	mCopyFlag = false;

	priv->playback_state = false; /* playback state to false when stopped */

	return 0;
}

static snd_pcm_uframes_t mtk_pcm_I2S0dl1_pointer(struct snd_pcm_substream
						 *substream)
{
	return get_mem_frame_index(substream,
		pI2S0dl1MemControl, Soc_Aud_Digital_Block_MEM_DL1);
}

static int mtk_pcm_I2S0dl1_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *hw_params)
{
	int ret = 0;

	substream->runtime->dma_bytes = params_buffer_bytes(hw_params);
	if (substream->runtime->dma_bytes <= GetPLaybackSramFullSize() &&
	    !pI2S0dl1MemControl->mAssignDRAM &&
	    AllocateAudioSram(&substream->runtime->dma_addr,
			      &substream->runtime->dma_area,
			      substream->runtime->dma_bytes,
			      substream,
			      params_format(hw_params), false) == 0) {
		SetHighAddr(Soc_Aud_Digital_Block_MEM_DL1, false, substream->runtime->dma_addr);
	} else {
		pr_debug("%s(), use DRAM\n", __func__);
		substream->runtime->dma_area = Dl1I2S0_Playback_dma_buf.area;
		substream->runtime->dma_addr = Dl1I2S0_Playback_dma_buf.addr;
		SetHighAddr(Soc_Aud_Digital_Block_MEM_DL1, true, substream->runtime->dma_addr);
		mPlaybackDramState = true;
		AudDrv_Emi_Clk_On();
	}

	set_mem_block(substream, hw_params, pI2S0dl1MemControl,
		      Soc_Aud_Digital_Block_MEM_DL1);

	pr_warn("dma_bytes = %zu dma_area = %p dma_addr = 0x%lx\n",
	       substream->runtime->dma_bytes, substream->runtime->dma_area,
	       (long)substream->runtime->dma_addr);

	return ret;
}

static int mtk_pcm_I2S0dl1_hw_free(struct snd_pcm_substream *substream)
{
	pr_warn("%s substream = %p\n", __func__, substream);
	if (mPlaybackDramState == true) {
		AudDrv_Emi_Clk_Off();
		mPlaybackDramState = false;
	} else
		freeAudioSram((void *)substream);

	return 0;
}

static struct snd_pcm_hw_constraint_list constraints_sample_rates = {
	.count = ARRAY_SIZE(soc_high_supported_sample_rates),
	.list = soc_high_supported_sample_rates,	/* TODO: KC: need check this!!!!!!!!!! */
	.mask = 0,
};

static int mtk_pcm_I2S0dl1_open(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	I2S0dl1_substream = substream;

	mPlaybackDramState = false;

	pr_warn("%s: mtk_I2S0dl1_hardware.buffer_bytes_max = %zu mPlaybackDramState = %d\n",
			__func__,
			mtk_I2S0dl1_hardware.buffer_bytes_max,
			mPlaybackDramState);
	runtime->hw = mtk_I2S0dl1_hardware;

	AudDrv_Clk_On();

	memcpy((void *)(&(runtime->hw)), (void *)&mtk_I2S0dl1_hardware,
	       sizeof(struct snd_pcm_hardware));
	pI2S0dl1MemControl = Get_Mem_ControlT(Soc_Aud_Digital_Block_MEM_DL1);

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &constraints_sample_rates);

	if (ret < 0)
		pr_warn("snd_pcm_hw_constraint_integer failed\n");

	if (ret < 0) {
		pr_err("ret < 0 mtk_pcm_I2S0dl1_close\n");
		mtk_pcm_I2S0dl1_close(substream);
		return ret;
	}

	return 0;
}

static int mtk_pcm_I2S0dl1_close(struct snd_pcm_substream *substream)
{
	struct mt_pcm_i2s0Dl1_priv *priv = dev_get_drvdata(mDev);
	pr_warn("%s\n", __func__);

	I2S0dl1_substream = NULL;

	if (is_irq_from_ext_module()) {
		ext_sync_signal_lock();
		ext_sync_signal_unlock();
	}

	if (mPrepareDone == true) {
		SetIntfConnection(Soc_Aud_InterCon_DisConnect, Soc_Aud_AFE_IO_Block_MEM_DL1,
				Soc_Aud_AFE_IO_Block_I2S1_DAC);
		SetIntfConnection(Soc_Aud_InterCon_DisConnect, Soc_Aud_AFE_IO_Block_MEM_DL1,
				Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
		if (!mt_soc_i2s1_clk_always_on()) {
			/* stop DAC output */
			SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, false);
			if (GetI2SDacEnable() == false)
				SetI2SDacEnable(false);

			RemoveMemifSubStream(Soc_Aud_Digital_Block_MEM_DL1, substream);

			if (mI2S0dl1_hdoutput_control == true) {
				pr_warn("%s mI2S0dl1_hdoutput_control == %d\n", __func__,
				       mI2S0dl1_hdoutput_control);
				/* here to close APLL */
				if (!mtk_soc_always_hd) {
					DisableAPLLTunerbySampleRate(substream->runtime->rate);
					DisableALLbySampleRate(substream->runtime->rate);
				}
			}
			EnableAfe(false);
		}
		mPrepareDone = false;
	}

	RemoveMemifSubStream(Soc_Aud_Digital_Block_MEM_DL1, substream);

	irq1_cnt = 0;	/* reset irq1_cnt */

	AudDrv_Clk_Off();
	vcore_dvfs(&vcore_dvfs_enable, true);
	priv->playback_state = false; /* playback state to false when closed */
	return 0;
}

static int mtk_pcm_I2S0dl1_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	bool mI2SWLen;

	if (mt_aud_i2s1_bit_width()){
		mI21SWLen = Soc_Aud_I2S_WLEN_WLEN_32BITS;
	} else
		mI21SWLen = Soc_Aud_I2S_WLEN_WLEN_16BITS;

	pr_warn("%s: mPrepareDone = %d, format = %d, SNDRV_PCM_FORMAT_S32_LE = %d, SNDRV_PCM_FORMAT_U32_LE = %d, sample rate = %d\n",
			       __func__,
			       mPrepareDone,
			       runtime->format,
			       SNDRV_PCM_FORMAT_S32_LE, SNDRV_PCM_FORMAT_U32_LE,
			       substream->runtime->rate);

	if (mPrepareDone == false) {
		SetMemifSubStream(Soc_Aud_Digital_Block_MEM_DL1, substream);

		if (runtime->format == SNDRV_PCM_FORMAT_S32_LE ||
		    runtime->format == SNDRV_PCM_FORMAT_U32_LE) {
			SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_DL1,
						     AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA);
			SetConnectionFormat(OUTPUT_DATA_FORMAT_24BIT, Soc_Aud_AFE_IO_Block_I2S1_DAC);
			SetConnectionFormat(OUTPUT_DATA_FORMAT_24BIT, Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
			mI2SWLen = Soc_Aud_I2S_WLEN_WLEN_32BITS;
		} else {
			SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_DL1, AFE_WLEN_16_BIT);
			SetConnectionFormat(OUTPUT_DATA_FORMAT_16BIT, Soc_Aud_AFE_IO_Block_I2S1_DAC);
			SetConnectionFormat(OUTPUT_DATA_FORMAT_16BIT, Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
			mI2SWLen = Soc_Aud_I2S_WLEN_WLEN_16BITS;
		}
		SetIntfConnection(Soc_Aud_InterCon_Connection, Soc_Aud_AFE_IO_Block_MEM_DL1,
				Soc_Aud_AFE_IO_Block_I2S1_DAC);
		SetIntfConnection(Soc_Aud_InterCon_Connection, Soc_Aud_AFE_IO_Block_MEM_DL1,
				Soc_Aud_AFE_IO_Block_I2S1_DAC_2);

		if (mI2S0dl1_hdoutput_control == true) {
			pr_warn("%s mI2S0dl1_hdoutput_control == %d\n", __func__,
			       mI2S0dl1_hdoutput_control);

			/* here to open APLL */
			if (!mtk_soc_always_hd) {
				EnableALLbySampleRate(runtime->rate);
				EnableAPLLTunerbySampleRate(runtime->rate);
			}

			if (!mt_soc_i2s1_clk_always_on()) {
				SetCLkMclk(Soc_Aud_I2S1, runtime->rate); /* select I2S */
				SetCLkMclk(Soc_Aud_I2S3, runtime->rate);

				/* EnableI2SDivPower(AUDIO_APLL12_DIV1, true); */
				/* EnableI2SDivPower(AUDIO_APLL12_DIV3, true); */

				EnableI2SCLKDiv(Soc_Aud_I2S1_MCKDIV, true);
				EnableI2SCLKDiv(Soc_Aud_I2S3_MCKDIV, true);
			}
		}

		/* start I2S DAC out */
		if (!mt_soc_i2s1_clk_always_on()) {
			if (GetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC) == false) {
				SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, true);
				SetI2SDacOut(substream->runtime->rate, mI2S0dl1_hdoutput_control, mI2SWLen);
				SetI2SDacEnable(true);
			} else {
				SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, true);
			}
		}

		EnableAfe(true);
		mPrepareDone = true;
	}
	return 0;
}

static int mtk_pcm_I2S0dl1_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mt_pcm_i2s0Dl1_priv *priv = dev_get_drvdata(mDev);
	pr_warn("%s\n", __func__);

	/* here to set interrupt */
	irq_add_substream_user(substream,
			       irq_request_number(Soc_Aud_Digital_Block_MEM_DL1),
			       substream->runtime->rate,
			       irq1_cnt ? irq1_cnt : substream->runtime->period_size);
	irq_user_id = substream;

	SetSampleRate(Soc_Aud_Digital_Block_MEM_DL1, runtime->rate);
	SetChannels(Soc_Aud_Digital_Block_MEM_DL1, runtime->channels);
	SetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL1, true);

	EnableAfe(true);
	priv->playback_state = true; /* set playback true when started playing */
	return 0;
}

static int mtk_pcm_I2S0dl1_trigger(struct snd_pcm_substream *substream, int cmd)
{
	/* pr_warn("mtk_pcm_I2S0dl1_trigger cmd = %d\n", cmd); */

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		return mtk_pcm_I2S0dl1_start(substream);
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return mtk_pcm_I2S0dl1_stop(substream);
	}
	return -EINVAL;
}

static int mtk_pcm_I2S0dl1_copy(struct snd_pcm_substream *substream,
				int channel, snd_pcm_uframes_t pos,
				void __user *dst, snd_pcm_uframes_t count)
{
	vcore_dvfs(&vcore_dvfs_enable, false);

	if (!mCopyFlag) {
		copy_count = count;
		mCopyFlag = true;
	}

	return mtk_memblk_copy(substream, channel, pos, dst, count, pI2S0dl1MemControl, Soc_Aud_Digital_Block_MEM_DL1);
}

static int mtk_pcm_I2S0dl1_silence(struct snd_pcm_substream *substream,
				   int channel, snd_pcm_uframes_t pos,
				   snd_pcm_uframes_t count)
{
	pr_warn("%s\n", __func__);
	return 0; /* do nothing */
}

static void *dummy_page[2];

static struct page *mtk_I2S0dl1_pcm_page(struct snd_pcm_substream *substream,
					 unsigned long offset)
{
	pr_warn("%s\n", __func__);
	return virt_to_page(dummy_page[substream->stream]); /* the same page */
}

static struct snd_pcm_ops mtk_I2S0dl1_ops = {
	.open =     mtk_pcm_I2S0dl1_open,
	.close =    mtk_pcm_I2S0dl1_close,
	.ioctl =    snd_pcm_lib_ioctl,
	.hw_params =    mtk_pcm_I2S0dl1_hw_params,
	.hw_free =  mtk_pcm_I2S0dl1_hw_free,
	.prepare =  mtk_pcm_I2S0dl1_prepare,
	.trigger =  mtk_pcm_I2S0dl1_trigger,
	.pointer =  mtk_pcm_I2S0dl1_pointer,
	.copy =     mtk_pcm_I2S0dl1_copy,
	.silence =  mtk_pcm_I2S0dl1_silence,
	.page =     mtk_I2S0dl1_pcm_page,
	.mmap =     mtk_pcm_mmap,
};

static struct snd_soc_platform_driver mtk_I2S0dl1_soc_platform = {
	.ops        = &mtk_I2S0dl1_ops,
	.probe      = mtk_afe_I2S0dl1_probe,
};

static int mtk_I2S0dl1_probe(struct platform_device *pdev)
{
	struct mt_pcm_i2s0Dl1_priv *priv;
	pr_debug("%s\n", __func__);

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);

	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	if (pdev->dev.of_node)
		dev_set_name(&pdev->dev, "%s", MT_SOC_I2S0DL1_PCM);

	priv = devm_kzalloc(&pdev->dev, sizeof(struct mt_pcm_i2s0Dl1_priv), GFP_KERNEL);
	if (unlikely(!priv)) {
		pr_err("%s failed to allocate private data\n", __func__);
		return -ENOMEM;
	}

	priv->playback_state = DEFAULT_PLAYBACK_STATE;

	dev_set_drvdata(&pdev->dev, priv);

	pr_warn("%s: dev name %s\n", __func__, dev_name(&pdev->dev));

	mDev = &pdev->dev;

	return snd_soc_register_platform(&pdev->dev,
					 &mtk_I2S0dl1_soc_platform);
}

static int mtk_afe_I2S0dl1_probe(struct snd_soc_platform *platform)
{
	pr_debug("mtk_afe_I2S0dl1_probe\n");
	snd_soc_add_platform_controls(platform, Audio_snd_I2S0dl1_controls,
				      ARRAY_SIZE(Audio_snd_I2S0dl1_controls));
	/* allocate dram */
	Dl1I2S0_Playback_dma_buf.area = dma_alloc_coherent(platform->dev,
						SOC_HIFI_BUFFER_SIZE,
						&Dl1I2S0_Playback_dma_buf.addr,
						GFP_KERNEL | GFP_DMA);
	if (!Dl1I2S0_Playback_dma_buf.area)
		return -ENOMEM;

	Dl1I2S0_Playback_dma_buf.bytes = SOC_HIFI_BUFFER_SIZE;
	pr_debug("area = %p\n", Dl1I2S0_Playback_dma_buf.area);

	return 0;
}

static int mtk_I2S0dl1_remove(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_soc_pcm_dl1_i2s0Dl1_of_ids[] = {
	{ .compatible = "mediatek,mt_soc_pcm_dl1_i2s0dl1", },
	{}
};
#endif

static struct platform_driver mtk_I2S0dl1_driver = {
	.driver = {
		.name = MT_SOC_I2S0DL1_PCM,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mt_soc_pcm_dl1_i2s0Dl1_of_ids,
#endif
	},
	.probe = mtk_I2S0dl1_probe,
	.remove = mtk_I2S0dl1_remove,
};

#ifndef CONFIG_OF
static struct platform_device *soc_mtkI2S0dl1_dev;
#endif

static int __init mtk_I2S0dl1_soc_platform_init(void)
{
	int ret;

	pr_warn("%s\n", __func__);
#ifndef CONFIG_OF
	soc_mtkI2S0dl1_dev = platform_device_alloc(MT_SOC_I2S0DL1_PCM, -1);
	if (!soc_mtkI2S0dl1_dev)
		return -ENOMEM;

	ret = platform_device_add(soc_mtkI2S0dl1_dev);
	if (ret != 0) {
		platform_device_put(soc_mtkI2S0dl1_dev);
		return ret;
	}
#endif

	ret = platform_driver_register(&mtk_I2S0dl1_driver);

	return ret;

}
module_init(mtk_I2S0dl1_soc_platform_init);

static void __exit mtk_I2S0dl1_soc_platform_exit(void)
{
	pr_warn("%s\n", __func__);

	platform_driver_unregister(&mtk_I2S0dl1_driver);
}
module_exit(mtk_I2S0dl1_soc_platform_exit);

MODULE_DESCRIPTION("AFE PCM module platform driver");
MODULE_LICENSE("GPL");
