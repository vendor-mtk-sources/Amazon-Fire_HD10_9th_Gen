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
 *   uldlloopback.c
 *
 * Project:
 * --------
 *   MT6595  Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio uldlloopback
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
#include "mtk-auddrv-common.h"
#include "mtk-soc-pcm-common.h"
#include "mtk-auddrv-def.h"
#include "mtk-auddrv-afe.h"
#include "mtk-auddrv-ana.h"
#include "mtk-auddrv-clk.h"
#include "mtk-auddrv-kernel.h"
#include "mtk-soc-afe-control.h"
#include "mtk-soc-pcm-platform.h"
#include "mtk-soc-analog-type.h"
#include "mtk-soc-digital-type.h"

/*
 *    function implementation
 */

static int m_input_use_single_ch;
static int m_input_use_lch;
extern int mt_soc_i2s1_clk_always_on(void);
extern int mt_soc_i2s2_clk_always_on(void);

enum {
	AP_LOOPBACK_NONE = 0,
	AP_LOOPBACK_AMIC_TO_SPK,
	AP_LOOPBACK_AMIC_TO_HP,
	AP_LOOPBACK_HEADSET_MIC_TO_SPK,
	AP_LOOPBACK_HEADSET_MIC_TO_HP,
};

struct mt_pcm_uldlloopback_priv {
	uint32_t ap_loopback_type;
};

static int ap_loopback_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	/* struct mt_pcm_uldlloopback_priv *priv = snd_soc_component_get_drvdata(component); */
	struct mt_pcm_uldlloopback_priv *priv = dev_get_drvdata(component->dev);

	ucontrol->value.integer.value[0] = priv->ap_loopback_type;
	return 0;
}

static int ap_loopback_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	/* struct mt_pcm_uldlloopback_priv *priv = snd_soc_component_get_drvdata(component); */
	struct mt_pcm_uldlloopback_priv *priv = dev_get_drvdata(component->dev);
	uint32_t sample_rate = 48000;
	long set_value = ucontrol->value.integer.value[0];

	if (priv->ap_loopback_type == set_value) {
		pr_debug("%s dummy operation for %u", __func__, priv->ap_loopback_type);
		return 0;
	}

	if (priv->ap_loopback_type != AP_LOOPBACK_NONE) {

		SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_IN_ADC, false);
		if (GetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_IN_ADC) == false)
			set_adc_enable(false);

		SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, false);
		if (GetI2SDacEnable() == false)
			SetI2SDacEnable(false);

		SetConnection(Soc_Aud_InterCon_DisConnect, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O03);
		SetConnection(Soc_Aud_InterCon_DisConnect, Soc_Aud_InterConnectionInput_I04,
			Soc_Aud_InterConnectionOutput_O04);

		EnableAfe(false);

		AudDrv_Clk_Off();
		AudDrv_ADC_Clk_Off();
		AudDrv_ANA_Clk_Off();
	}

	if (set_value == AP_LOOPBACK_AMIC_TO_SPK ||
	    set_value == AP_LOOPBACK_AMIC_TO_HP ||
	    set_value == AP_LOOPBACK_HEADSET_MIC_TO_SPK ||
	    set_value == AP_LOOPBACK_HEADSET_MIC_TO_HP) {

		AudDrv_ANA_Clk_On();
		AudDrv_Clk_On();
		AudDrv_ADC_Clk_On();

		SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_DL1, AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA);
		SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_DL2, AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA);
		SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_24BIT, Soc_Aud_InterConnectionOutput_O03);
		SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_24BIT, Soc_Aud_InterConnectionOutput_O04);

		SetSampleRate(Soc_Aud_Digital_Block_MEM_I2S,  sample_rate);

		/* configure downlink */
		if (GetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC) == false) {
			SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, true);
			SetI2SDacOut(sample_rate, false, OUTPUT_DATA_FORMAT_24BIT);
			SetI2SDacEnable(true);
		} else
			SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, true);

		/* configure uplink */
		if (GetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_IN_ADC) == false) {
			set_adc_in(sample_rate);
			SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_IN_ADC, true);
			set_adc_enable(true);
		} else
			SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_IN_ADC, true);

		SetConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O03);
		SetConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I04,
			Soc_Aud_InterConnectionOutput_O04);

		EnableAfe(true);
	}

	priv->ap_loopback_type = ucontrol->value.integer.value[0];

	return 0;
}

static const char *const ap_loopback_function[] = {
	"AP_LOOPBACK_NONE",
	"AP_LOOPBACK_AMIC_TO_SPK",
	"AP_LOOPBACK_AMIC_TO_HP",
	"AP_LOOPBACK_HEADSET_MIC_TO_SPK",
	"AP_LOOPBACK_HEADSET_MIC_TO_HP",
};

static const struct soc_enum mt_pcm_loopback_control_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ap_loopback_function), ap_loopback_function),
};

static const struct snd_kcontrol_new mt_pcm_loopback_controls[] = {
	SOC_ENUM_EXT("AP_Loopback_Select", mt_pcm_loopback_control_enum[0], ap_loopback_get,
		     ap_loopback_set),
};

static const char * const LR_channel_switch[] = {"Off", "Left", "Right"};
static const char * const switch_texts[] = {"Off", "On"};

static const struct soc_enum input_use_sigle_ch_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(LR_channel_switch), LR_channel_switch),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(switch_texts), switch_texts),
};

static int lpbk_in_use_lch_get(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("m_input_use_lch = %d\n", m_input_use_lch);
	ucontrol->value.integer.value[0] = m_input_use_lch;
	return 0;
}

static int lpbk_in_use_lch_set(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(switch_texts)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}

	m_input_use_lch = ucontrol->value.integer.value[0];

	return 0;
}

static int lpbk_in_use_single_ch_get(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("m_input_use_single_ch = %d\n", m_input_use_single_ch);
	ucontrol->value.integer.value[0] = m_input_use_single_ch;
	return 0;
}

static int lpbk_in_use_single_ch_set(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	m_input_use_single_ch = ucontrol->value.integer.value[0];
	pr_debug("%s(), m_input_use_single_ch = %d\n", __func__, m_input_use_single_ch);

	AudDrv_Clk_On();

	switch (m_input_use_single_ch) {
	case 0:
		SetIntfConnection(Soc_Aud_InterCon_DisConnect,
				Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S3);
		SetIntfConnection(Soc_Aud_InterCon_DisConnect,
				Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S1_DAC);
		SetIntfConnection(Soc_Aud_InterCon_DisConnect,
				Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
		SetIntfConnection(Soc_Aud_InterCon_DisConnect,
				Soc_Aud_AFE_IO_Block_ADDA_UL_RCH, Soc_Aud_AFE_IO_Block_I2S3);
		SetIntfConnection(Soc_Aud_InterCon_DisConnect,
				Soc_Aud_AFE_IO_Block_ADDA_UL_RCH, Soc_Aud_AFE_IO_Block_I2S1_DAC);
		SetIntfConnection(Soc_Aud_InterCon_DisConnect,
				Soc_Aud_AFE_IO_Block_ADDA_UL_RCH, Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
		break;
	case 1:
		SetIntfConnection(Soc_Aud_InterCon_DisConnect,
				Soc_Aud_AFE_IO_Block_ADDA_UL_RCH, Soc_Aud_AFE_IO_Block_I2S3);
		SetIntfConnection(Soc_Aud_InterCon_DisConnect,
				Soc_Aud_AFE_IO_Block_ADDA_UL_RCH, Soc_Aud_AFE_IO_Block_I2S1_DAC);
		SetIntfConnection(Soc_Aud_InterCon_DisConnect,
				Soc_Aud_AFE_IO_Block_ADDA_UL_RCH, Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
		SetIntfConnection(Soc_Aud_InterCon_Connection,
				Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S3);
		SetIntfConnection(Soc_Aud_InterCon_Connection,
				Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S1_DAC);
		SetIntfConnection(Soc_Aud_InterCon_Connection,
				Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
		break;
	case 2:
		SetIntfConnection(Soc_Aud_InterCon_DisConnect,
				Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S3);
		SetIntfConnection(Soc_Aud_InterCon_DisConnect,
				Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S1_DAC);
		SetIntfConnection(Soc_Aud_InterCon_DisConnect,
				Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
		SetIntfConnection(Soc_Aud_InterCon_Connection,
				Soc_Aud_AFE_IO_Block_ADDA_UL_RCH, Soc_Aud_AFE_IO_Block_I2S3);
		SetIntfConnection(Soc_Aud_InterCon_Connection,
				Soc_Aud_AFE_IO_Block_ADDA_UL_RCH, Soc_Aud_AFE_IO_Block_I2S1_DAC);
		SetIntfConnection(Soc_Aud_InterCon_Connection,
				Soc_Aud_AFE_IO_Block_ADDA_UL_RCH, Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
		break;
	default:
		pr_err("%s, control error, return -EINVAL\n", __func__);
		AudDrv_Clk_Off();
		return -EINVAL;
	}

	AudDrv_Clk_Off();
	return 0;
}

static const struct snd_kcontrol_new lpbk_controls[] = {
	SOC_ENUM_EXT("loopback_use_single_input", input_use_sigle_ch_enum[0],
		     lpbk_in_use_single_ch_get, lpbk_in_use_single_ch_set),
	SOC_ENUM_EXT("LPBK_IN_USE_LCH", input_use_sigle_ch_enum[1],
		     lpbk_in_use_lch_get, lpbk_in_use_lch_set),
};

static int mtk_uldlloopback_probe(struct platform_device *pdev);
static int mtk_uldlloopbackpcm_close(struct snd_pcm_substream *substream);
static int mtk_afe_uldlloopback_probe(struct snd_soc_platform *platform);

static struct snd_pcm_hardware mtk_uldlloopback_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP |
	SNDRV_PCM_INFO_INTERLEAVED |
	SNDRV_PCM_INFO_RESUME |
	SNDRV_PCM_INFO_MMAP_VALID),
	.formats =      SND_SOC_STD_MT_FMTS,
	.rates =        SOC_HIGH_USE_RATE,
	.rate_min =     SOC_NORMAL_USE_RATE_MIN,
	.rate_max =     SOC_NORMAL_USE_RATE_MAX,
	.channels_min =     SOC_NORMAL_USE_CHANNELS_MIN,
	.channels_max =     SOC_NORMAL_USE_CHANNELS_MAX,
	.buffer_bytes_max = Dl1_MAX_BUFFER_SIZE,
	.period_bytes_max = MAX_PERIOD_SIZE,
	.periods_min =      MIN_PERIOD_SIZE,
	.periods_max =      MAX_PERIOD_SIZE,
	.fifo_size =        0,
};

static struct snd_pcm_hw_constraint_list constraints_sample_rates = {
	.count = ARRAY_SIZE(soc_high_supported_sample_rates),
	.list = soc_high_supported_sample_rates,
	.mask = 0,
};

static int mtk_uldlloopback_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;

	pr_debug("%s, stream = %s\n",
		 __func__,
		 substream->stream == SNDRV_PCM_STREAM_PLAYBACK ?
		 "SNDRV_PCM_STREAM_PLAYBACK" : "SNDRV_PCM_STREAM_CAPTURE");

	AudDrv_Clk_On();

	runtime->hw = mtk_uldlloopback_hardware;
	memcpy((void *)(&(runtime->hw)), (void *)&mtk_uldlloopback_hardware,
	       sizeof(struct snd_pcm_hardware));

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &constraints_sample_rates);
	ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

	if (ret < 0)
		pr_warn("snd_pcm_hw_constraint_integer failed\n");

	/* print for hw pcm information */
	pr_debug("%s, runtime rate = %d channels = %d\n",
		__func__, runtime->rate, runtime->channels);
	runtime->hw.info |= SNDRV_PCM_INFO_INTERLEAVED;
	runtime->hw.info |= SNDRV_PCM_INFO_NONINTERLEAVED;

	if (ret < 0) {
		pr_err("mtk_uldlloopbackpcm_close\n");
		mtk_uldlloopbackpcm_close(substream);
		return ret;
	}

	return 0;
}

static int mtk_uldlloopbackpcm_close(struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		pr_err("%s  with SNDRV_PCM_STREAM_CAPTURE\n", __func__);
		AudDrv_Clk_Off();
		return 0;
	}

	pr_debug("%s\n", __func__);

	/* interconnection setting */
	SetIntfConnection(Soc_Aud_InterCon_DisConnect,
			Soc_Aud_AFE_IO_Block_ADDA_UL, Soc_Aud_AFE_IO_Block_I2S3);
	SetIntfConnection(Soc_Aud_InterCon_DisConnect,
			Soc_Aud_AFE_IO_Block_ADDA_UL, Soc_Aud_AFE_IO_Block_I2S1_DAC);
	SetIntfConnection(Soc_Aud_InterCon_DisConnect,
			Soc_Aud_AFE_IO_Block_ADDA_UL, Soc_Aud_AFE_IO_Block_I2S1_DAC_2);

	SetIntfConnection(Soc_Aud_InterCon_DisConnect,
			Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S3);
	SetIntfConnection(Soc_Aud_InterCon_DisConnect,
			Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S1_DAC);
	SetIntfConnection(Soc_Aud_InterCon_DisConnect,
			Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S1_DAC_2);

	SetMemoryPathEnable(Soc_Aud_Digital_Block_ADDA_UL, false);
	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_ADDA_UL) == false)
		set_adc_enable(false);

	/* stop DAC output */
	SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, false);
	if (GetI2SDacEnable() == false)
		SetI2SDacEnable(false);

	/* stop I2S */
	Afe_Set_Reg(AFE_I2S_CON3, 0x0, 0x1);
	Afe_Set_Reg(AFE_I2S_CON, 0x0, 0x1);
	if (!mt_soc_i2s1_clk_always_on())
		Afe_Set_Reg(AFE_I2S_CON1, 0x0, 0x1); /* disable for alway on I2S1 bck/mck */
	if (!mt_soc_i2s2_clk_always_on())
		Afe_Set_Reg(AFE_I2S_CON2, 0x0, 0x1); /* disable for alway on I2S2 bck/mck */


	EnableAfe(false);

	AudDrv_Clk_Off();
	return 0;
}

static int mtk_uldlloopbackpcm_trigger(struct snd_pcm_substream *substream,
				       int cmd)
{
	pr_warn("%s cmd = %d\n", __func__, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		break;
	}
	return -EINVAL;
}

static int mtk_uldlloopback_pcm_copy(struct snd_pcm_substream *substream,
				     int channel, snd_pcm_uframes_t pos,
				     void __user *dst, snd_pcm_uframes_t count)
{
	count = count << 2;
	return 0;
}

static int mtk_uldlloopback_silence(struct snd_pcm_substream *substream,
				    int channel, snd_pcm_uframes_t pos,
				    snd_pcm_uframes_t count)
{
	pr_warn("dummy_pcm_silence\n");
	return 0; /* do nothing */
}


static void *dummy_page[2];

static struct page *mtk_uldlloopback_page(struct snd_pcm_substream *substream,
					  unsigned long offset)
{
	pr_warn("dummy_pcm_page\n");
	return virt_to_page(dummy_page[substream->stream]); /* the same page */
}

static int mtk_uldlloopback_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	/* unsigned int eSamplingRate = SampleRateTransform(runtime->rate); */
	/* unsigned int dVoiceModeSelect = 0; */
	/* unsigned int Audio_I2S_Dac = 0; */
	unsigned int u32AudioI2S = 0;
	bool mI2SWLen;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		pr_debug("%s  SNDRV_PCM_STREAM_CAPTURE return\n", __func__);
		return 0;
	}

	pr_debug("%s rate = %d\n", __func__, runtime->rate);

	Afe_Set_Reg(AFE_TOP_CON0, 0x00000000, 0xffffffff);
	if (runtime->format == SNDRV_PCM_FORMAT_S32_LE ||
	    runtime->format == SNDRV_PCM_FORMAT_U32_LE) {
		SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_DL1,
					     AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA);
		SetConnectionFormat(OUTPUT_DATA_FORMAT_24BIT,
				Soc_Aud_AFE_IO_Block_I2S1_DAC);
		SetConnectionFormat(OUTPUT_DATA_FORMAT_24BIT,
				Soc_Aud_AFE_IO_Block_I2S3);
		SetConnectionFormat(OUTPUT_DATA_FORMAT_24BIT,
				Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
		mI2SWLen = Soc_Aud_I2S_WLEN_WLEN_32BITS;
	} else {
		SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_DL1, AFE_WLEN_16_BIT);
		SetConnectionFormat(OUTPUT_DATA_FORMAT_16BIT,
				Soc_Aud_AFE_IO_Block_I2S1_DAC);
		SetConnectionFormat(OUTPUT_DATA_FORMAT_16BIT,
				Soc_Aud_AFE_IO_Block_I2S3);
		SetConnectionFormat(OUTPUT_DATA_FORMAT_16BIT,
				Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
		mI2SWLen = Soc_Aud_I2S_WLEN_WLEN_16BITS;
	}

	/* interconnection setting */
	if (m_input_use_lch == 1) {
		SetIntfConnection(Soc_Aud_InterCon_Connection,
				Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S3);
		SetIntfConnection(Soc_Aud_InterCon_Connection,
				Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S1_DAC);
		SetIntfConnection(Soc_Aud_InterCon_Connection,
				Soc_Aud_AFE_IO_Block_ADDA_UL_LCH, Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
	} else {
		SetIntfConnection(Soc_Aud_InterCon_Connection,
				Soc_Aud_AFE_IO_Block_ADDA_UL, Soc_Aud_AFE_IO_Block_I2S3);
		SetIntfConnection(Soc_Aud_InterCon_Connection,
				Soc_Aud_AFE_IO_Block_ADDA_UL, Soc_Aud_AFE_IO_Block_I2S1_DAC);
		SetIntfConnection(Soc_Aud_InterCon_Connection,
				Soc_Aud_AFE_IO_Block_ADDA_UL, Soc_Aud_AFE_IO_Block_I2S1_DAC_2);

	}

	Afe_Set_Reg(AFE_ADDA_TOP_CON0, 0, 0x1); /* Using Internal ADC */

	u32AudioI2S |= Soc_Aud_LOW_JITTER_CLOCK << 12; /* Low jitter mode */
	u32AudioI2S |= SampleRateTransform(runtime->rate, Soc_Aud_Digital_Block_I2S_OUT_2) << 8;
	u32AudioI2S |= Soc_Aud_I2S_FORMAT_I2S << 3; /* us3 I2s format */
	u32AudioI2S |= mI2SWLen << 1;
	pr_warn("u32AudioI2S= 0x%x\n", u32AudioI2S);
	Afe_Set_Reg(AFE_I2S_CON3, u32AudioI2S | 1, AFE_MASK_ALL);

	/* start I2S DAC out */
	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC) == false) {
		SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, true);
		SetI2SDacOut(substream->runtime->rate, false, mI2SWLen);
		SetI2SDacEnable(true);
	} else
		SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, true);

	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_ADDA_UL) == false) {
		SetMemoryPathEnable(Soc_Aud_Digital_Block_ADDA_UL, true);
		set_adc_in(substream->runtime->rate);
		set_adc_enable(true);
	} else
		SetMemoryPathEnable(Soc_Aud_Digital_Block_ADDA_UL, true);

	EnableAfe(true);

	return 0;
}

static int mtk_uldlloopback_pcm_hw_params(struct snd_pcm_substream *substream,
					  struct snd_pcm_hw_params *hw_params)
{
	int ret = 0;

	PRINTK_AUDDRV("mtk_uldlloopback_pcm_hw_params\n");
	return ret;
}

static int mtk_uldlloopback_pcm_hw_free(struct snd_pcm_substream *substream)
{
	PRINTK_AUDDRV("mtk_uldlloopback_pcm_hw_free\n");
	return snd_pcm_lib_free_pages(substream);
}

static struct snd_pcm_ops mtk_afe_ops = {
	.open =     mtk_uldlloopback_open,
	.close =    mtk_uldlloopbackpcm_close,
	.ioctl =    snd_pcm_lib_ioctl,
	.hw_params =    mtk_uldlloopback_pcm_hw_params,
	.hw_free =  mtk_uldlloopback_pcm_hw_free,
	.prepare =  mtk_uldlloopback_pcm_prepare,
	.trigger =  mtk_uldlloopbackpcm_trigger,
	.copy =     mtk_uldlloopback_pcm_copy,
	.silence =  mtk_uldlloopback_silence,
	.page =     mtk_uldlloopback_page,
};

static struct snd_soc_platform_driver mtk_soc_dummy_platform = {
	.ops        = &mtk_afe_ops,
	.probe      = mtk_afe_uldlloopback_probe,
};

static int mtk_uldlloopback_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mt_pcm_uldlloopback_priv *priv;

	pr_warn("mtk_uldlloopback_probe\n");

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	if (pdev->dev.of_node)
		dev_set_name(&pdev->dev, "%s", MT_SOC_ULDLLOOPBACK_PCM);

	priv = devm_kzalloc(dev, sizeof(struct mt_pcm_uldlloopback_priv), GFP_KERNEL);
	if (unlikely(!priv)) {
		pr_debug("%s failed to allocate private data\n", __func__);
		return -ENOMEM;
	}

	dev_set_drvdata(dev, priv);

	pr_warn("%s: dev name %s\n", __func__, dev_name(&pdev->dev));
	return snd_soc_register_platform(&pdev->dev,
					 &mtk_soc_dummy_platform);
}

static int mtk_afe_uldlloopback_probe(struct snd_soc_platform *platform)
{
	pr_warn("mtk_afe_uldlloopback_probe\n");

	snd_soc_add_platform_controls(platform, mt_pcm_loopback_controls,
			      ARRAY_SIZE(mt_pcm_loopback_controls));
	snd_soc_add_platform_controls(platform, lpbk_controls,
			      ARRAY_SIZE(lpbk_controls));
	return 0;
}


static int mtk_afe_uldlloopback_remove(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_soc_pcm_uldlloopback_of_ids[] = {
	{ .compatible = "mediatek,mt_soc_pcm_uldlloopback", },
	{}
};
#endif

static struct platform_driver mtk_afe_uldllopback_driver = {
	.driver = {
		.name = MT_SOC_ULDLLOOPBACK_PCM,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mt_soc_pcm_uldlloopback_of_ids,
#endif
	},
	.probe = mtk_uldlloopback_probe,
	.remove = mtk_afe_uldlloopback_remove,
};

#ifndef CONFIG_OF
static struct platform_device *soc_mtkafe_uldlloopback_dev;
#endif

static int __init mtk_soc_uldlloopback_platform_init(void)
{
	int ret = 0;

	pr_warn("%s\n", __func__);
#ifndef CONFIG_OF
	soc_mtkafe_uldlloopback_dev = platform_device_alloc(MT_SOC_ULDLLOOPBACK_PCM,
							    -1);
	if (!soc_mtkafe_uldlloopback_dev)
		return -ENOMEM;

	ret = platform_device_add(soc_mtkafe_uldlloopback_dev);
	if (ret != 0) {
		platform_device_put(soc_mtkafe_uldlloopback_dev);
		return ret;
	}
#endif
	ret = platform_driver_register(&mtk_afe_uldllopback_driver);

	return ret;

}
module_init(mtk_soc_uldlloopback_platform_init);

static void __exit mtk_soc_uldlloopback_platform_exit(void)
{

	pr_warn("%s\n", __func__);
	platform_driver_unregister(&mtk_afe_uldllopback_driver);
}
module_exit(mtk_soc_uldlloopback_platform_exit);

MODULE_DESCRIPTION("loopback module platform driver");
MODULE_LICENSE("GPL");
