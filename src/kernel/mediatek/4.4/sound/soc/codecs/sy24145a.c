/*
 * ASoC Driver for SY24145A
 *
 * Author:    XiaoTian Su <xiaotian.su@silergycorp.com>

 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "sy24145a.h"

#define DAP_RAM_ITEM_SIZE           (1+20) // address(1 byte) + data(20 byte)
#define DAP_RAM_ITEM_ALL           (BIT(DAP_RAM_ITEM_NUM)-1)
#define SY24145A_CLEAR_FAULT (0x00)

struct sy24145a_reg_switch {
	uint reg;
	uint mask;
	uint on_value;
	uint off_value;
};
static const struct sy24145a_reg sy24145a_reg_init_reset[] =
{
	{.reg = SY24145A_DC_SOFT_RESET,.val = 0x01			},	// Reset
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static struct i2c_client *i2c;

struct sy24145a_priv {
	struct regmap *regmap;
	int mclk_div;
	struct snd_soc_codec *codec;	//change component to codec
	unsigned int format;
	u8  dap_ram[DAP_RAM_ITEM_NUM][DAP_RAM_ITEM_SIZE];
	u32 dap_ram_update1, dap_ram_update2;
	struct mutex dap_ram_lock;
};

static struct sy24145a_priv *sy24145a_priv_data;


static void sy24145a_write_dap_ram_commands(const struct sy24145a_dap_ram_command *cmds, uint cnt)
{
	uint i;

	for ( i = 0 ; i < cnt ; ++i )
	{
		if ( cmds[i].item  < DAP_RAM_ITEM_NUM )
		{
			memcpy(sy24145a_priv_data->dap_ram[cmds[i].item], cmds[i].data, sy24145a_reg_size[cmds[i].data[0]] + 1);
			if(cmds[i].item<32)
				sy24145a_priv_data->dap_ram_update1 |= ( (u32)1 << cmds[i].item );
			else
			{
				sy24145a_priv_data->dap_ram_update2 |= ((u32)1 << (cmds[i].item-32));
			}
		}
	}
}

static int sy24145a_write_reg_list(struct snd_soc_codec *codec, const struct sy24145a_reg *reg_list, uint reg_cnt)	//change component to codec
{
	int ret, i;

	if ( reg_list == NULL || reg_cnt == 0 )
		return 0;

	for ( i = 0 ; i < reg_cnt ; ++i )
	{
		 ret = snd_soc_write(codec, reg_list[i].reg, reg_list[i].val); //change component to codec
		 if (ret < 0) return ret;
	}

	return 0;
}

static int sy24145a_dap_ram_update(struct snd_soc_codec *codec) //change component to codec
{
	struct sy24145a_priv *sy24145a;
	int ret, i, cnt;
	u32 update1, update2;

	sy24145a = snd_soc_codec_get_drvdata(codec);	//change component to codec

	update1 = sy24145a->dap_ram_update1;
	update2 = sy24145a->dap_ram_update2;
	if ( update1 == 0 && update2 == 0 )
		return 0;

	ret = snd_soc_update_bits(codec, SY24145A_SYS_CTL_REG1, SY24145A_I2C_ACCESS_RAM_MASK, SY24145A_I2C_ACCESS_RAM_ENABLE);
	if (ret < 0) return ret;

	i = 0;
	while ( update1||update2 )
	{
		if (i < 32)
		{
			if (update1 & 0x01)
			{
				cnt = sy24145a_reg_size[sy24145a->dap_ram[i][0]] + 1;
				ret = i2c_master_send(i2c, &(sy24145a->dap_ram[i][0]), cnt);
				if (ret < 0) return ret;
			}
			update1 >>= 1;
		}
		else
		{
			if (update2 & 0x01)
			{
				cnt = sy24145a_reg_size[sy24145a->dap_ram[i][0]] + 1;
				ret = i2c_master_send(i2c, &(sy24145a->dap_ram[i][0]), cnt);
				if (ret < 0) return ret;
			}
			update2 >>= 1;
		}
		++i;
	}

	ret = snd_soc_update_bits(codec, SY24145A_SYS_CTL_REG1, SY24145A_I2C_ACCESS_RAM_MASK, SY24145A_I2C_ACCESS_RAM_DISABLE);

	sy24145a->dap_ram_update1 = 0;
	sy24145a->dap_ram_update2 = 0;

	return ret;
}



/*
 *    _   _    ___   _      ___         _           _
 *   /_\ | |  / __| /_\    / __|___ _ _| |_ _ _ ___| |___
 *  / _ \| |__\__ \/ _ \  | (__/ _ \ ' \  _| '_/ _ \ (_-<
 * /_/ \_\____|___/_/ \_\  \___\___/_||_\__|_| \___/_/__/
 *
 */

static const DECLARE_TLV_DB_SCALE(sy24145a_vol_tlv_m, -12750, 50, 1);
static const DECLARE_TLV_DB_SCALE(sy24145a_vol_tlv,          -7950, 50, 1);



static const struct snd_kcontrol_new sy24145a_snd_controls[] = {


	SOC_SINGLE_TLV  ("Master Playback Volume", SY24145A_MAS_VOL, 0, 255, 0, sy24145a_vol_tlv_m),
	SOC_SINGLE_TLV  ("Ch1 Playback Volume", SY24145A_CH1_VOL, 0, 255, 0, sy24145a_vol_tlv),
	SOC_SINGLE_TLV  ("Ch2 Playback Volume", SY24145A_CH2_VOL, 0, 255, 0, sy24145a_vol_tlv),
	// switch
	SOC_SINGLE("Master Playback Switch", SY24145A_SOFT_MUTE, SY24145A_SOFT_MUTE_MASTER_SHIFT, 1, 1),
	SOC_SINGLE("Ch1 Playback Switch", SY24145A_SOFT_MUTE, SY24145A_SOFT_MUTE_CH1_SHIFT, 1, 1),
	SOC_SINGLE("Ch2 Playback Switch", SY24145A_SOFT_MUTE, SY24145A_SOFT_MUTE_CH2_SHIFT, 1, 1),
	SOC_SINGLE("EQ Switch", SY24145A_SYS_CTL_REG2, SY24145A_EQ_ENABLE_SHIFT, 1, 0),

};



/*
 *  __  __         _    _            ___      _
 * |  \/  |__ _ __| |_ (_)_ _  ___  |   \ _ _(_)_ _____ _ _
 * | |\/| / _` / _| ' \| | ' \/ -_) | |) | '_| \ V / -_) '_|
 * |_|  |_\__,_\__|_||_|_|_||_\___| |___/|_| |_|\_/\___|_|
 *
 */
static int sy24145a_set_dai_fmt(struct snd_soc_dai *dai, unsigned int format)
{
	struct sy24145a_priv *sy24145a;
	struct snd_soc_codec *codec = dai->codec;
	sy24145a = snd_soc_codec_get_drvdata(codec);

	sy24145a->format = format;

	return 0;
}

static int sy24145a_hw_params(struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params,
			struct snd_soc_dai *dai)
{
	u32  val = 0x00;
	bool err=  false;

	struct sy24145a_priv *sy24145a;
	struct snd_soc_codec *codec = dai->codec;
	sy24145a = snd_soc_codec_get_drvdata(codec);

	sy24145a_priv_data->codec = codec;

	switch (sy24145a->format & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		val |= 0x00;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		val |= 0x20;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		val |= 0x40;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		val |= 0x60;
		break;
	default:
		err = true;
		break;
	}

	switch (sy24145a->format & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		val |= 0x00;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		val |= 0x04;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		val |= 0x08;
		break;
	default:
		err = true;
		break;
	}

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		val |= 0x03;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		val |= 0x01;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		val   |= 0x00;
		break;
	default:
		err = true;
		break;
	}

	if ( !err )
		val |= 0x10;

	return ( snd_soc_write(codec, SY24145A_I2S_CONTROL, val) );

}


static int sy24145a_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	unsigned int val;

	struct sy24145a_priv *sy24145a;
	struct snd_soc_codec *codec = dai->codec;
	sy24145a = snd_soc_codec_get_drvdata(codec);

	val = (mute) ? SY24145A_SOFT_MUTE_ALL : 0;

	return ( snd_soc_update_bits(codec, SY24145A_SOFT_MUTE, SY24145A_SOFT_MUTE_ALL, val) );
}


static const struct snd_soc_dai_ops sy24145a_dai_ops = {
	.set_fmt		= sy24145a_set_dai_fmt,
	.hw_params		= sy24145a_hw_params,
	.mute_stream	= sy24145a_mute_stream,
};


static struct snd_soc_dai_driver sy24145a_dai = {
	.name		= "sy24145a-hifi",
	.playback	= {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates			= (SNDRV_PCM_RATE_32000|SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000|SNDRV_PCM_RATE_96000),
		.formats		= (SNDRV_PCM_FMTBIT_S16_LE|SNDRV_PCM_FMTBIT_S24_LE|SNDRV_PCM_FMTBIT_S32_LE ),
	},
	.ops	= &sy24145a_dai_ops,
};



/*
 *   ___         _          ___      _
 *  / __|___  __| |___ __  |   \ _ _(_)_ _____ _ _
 * | (__/ _ \/ _` / -_) _| | |) | '_| \ V / -_) '_|
 *  \___\___/\__,_\___\__| |___/|_| |_|\_/\___|_|
 *
 */

static int sy24145a_remove(struct snd_soc_codec *codec)
{
	struct sy24145a_priv *sy24145a;

	sy24145a = snd_soc_codec_get_drvdata(codec);

	return 0;
}


static int sy24145a_probe(struct snd_soc_codec *codec)
{
	struct sy24145a_priv *sy24145a;
	int ret;
	i2c = container_of(codec->dev, struct i2c_client, dev);

	sy24145a = snd_soc_codec_get_drvdata(codec);

	ret = sy24145a_write_reg_list(codec, sy24145a_reg_init_reset, ARRAY_SIZE(sy24145a_reg_init_reset));
	if (ret < 0) return ret;
	mdelay(5);

	ret = sy24145a_write_reg_list(codec, sy24145a_reg_init_mute, ARRAY_SIZE(sy24145a_reg_init_mute));
	if (ret < 0) return ret;
	mdelay(10);

	ret = sy24145a_write_reg_list(codec, sy24145a_reg_init_seq_0, ARRAY_SIZE(sy24145a_reg_init_seq_0));
	if (ret < 0) return ret;

	sy24145a->dap_ram_update1 = 0;
	sy24145a->dap_ram_update2 = 0;

	sy24145a_write_dap_ram_commands(sy24145a_dap_ram_init, ARRAY_SIZE(sy24145a_dap_ram_init));
	sy24145a_write_dap_ram_commands(sy24145a_dap_ram_eq_1, ARRAY_SIZE(sy24145a_dap_ram_eq_1)); 
	ret = sy24145a_dap_ram_update(codec);
	if (ret < 0) return ret;

	ret = sy24145a_write_reg_list(codec, sy24145a_reg_eq_1, ARRAY_SIZE(sy24145a_reg_eq_1));
	if (ret < 0) return ret;
	ret = sy24145a_write_reg_list(codec, sy24145a_reg_speq_ctrl, ARRAY_SIZE(sy24145a_reg_speq_ctrl));
	if (ret < 0) return ret;
	ret = sy24145a_write_reg_list(codec, sy24145a_reg_init_seq_1, ARRAY_SIZE(sy24145a_reg_init_seq_1));

	return ret;

}

void sy24145a_process_fault(struct snd_soc_codec *codec)
{
	unsigned int fault_reg;
	if (codec == NULL) {
		pr_err("%s: Can't process fault\n", __func__);
		return;
	}

	fault_reg = snd_soc_read(codec, SY24145A_ERR_STATUS_REG) & 0x7;
	if (fault_reg) {
		pr_warn("%s: fault_reg (0x2) %x", __func__, fault_reg);
		usleep_range(10000, 10200);
		snd_soc_write(codec, SY24145A_ERR_STATUS_REG, SY24145A_CLEAR_FAULT);
		pr_warn("%s: Processed Sy24145a Woofer AMP fault\n", __func__);
	}
}
EXPORT_SYMBOL(sy24145a_process_fault);

static const struct snd_soc_codec_driver soc_codec_dev_sy24145a = {
	.probe = sy24145a_probe,
	.remove = sy24145a_remove,
	.component_driver = {
		.controls = sy24145a_snd_controls,
		.num_controls = ARRAY_SIZE(sy24145a_snd_controls),
	}
};




/*
 *   ___ ___ ___   ___      _
 *  |_ _|_  ) __| |   \ _ _(_)_ _____ _ _
 *   | | / / (__  | |) | '_| \ V / -_) '_|
 *  |___/___\___| |___/|_| |_|\_/\___|_|
 *
 */

static const struct reg_default sy24145a_reg_defaults[] = {
	{ 0x07 , 0x00 },  // R7  - VOL_MASTER    -   mute
	{ 0x08 , 0x9F },  // R8  - VOL_CH1       -   0dB
	{ 0x09 , 0x9F },  // R9  - VOL_CH2       -   0dB
};


static bool sy24145a_reg_volatile(struct device *dev, unsigned int reg)
{
	switch (reg) {
		case SY24145A_CLK_CTL_REG:
		case SY24145A_DEV_ID_REG:
		case SY24145A_ERR_STATUS_REG:
		case SY24145A_DC_SOFT_RESET:
			return true;
	default:
			return false;
	}
}


static const struct of_device_id sy24145a_of_match[] = {
	{ .compatible = "silergy,sy24145a", },
	{ }
};
MODULE_DEVICE_TABLE(of, sy24145a_of_match);


static int sy24145a_reg_read(void *context, unsigned int reg,
	unsigned int *value)
{
	int ret;
	uint size, i;
	u8 send_buf[4] = {0};
	u8 recv_buf[4] = {0};
	struct i2c_client *client = context;
	struct i2c_msg msgs[2];
	struct device *dev = &client->dev;

	if ( reg > SY24145A_MAX_REGISTER )
		return -EINVAL;

	size = sy24145a_reg_size[reg];
	if (size == 0)
		return -EINVAL;
	if ( size > 4 )
		size = 4;

	send_buf[0]   = (u8)reg;

	msgs[0].addr  = client->addr;
	msgs[0].len   = 1;
	msgs[0].buf   = send_buf;
	msgs[0].flags = 0;

	msgs[1].addr  = client->addr;
	msgs[1].len   = size;
	msgs[1].buf   = recv_buf;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_err(dev, "Failed to register codec: %d\n", ret);
		return ret;
	} else if (ret != ARRAY_SIZE(msgs)) {
		dev_err(dev, "Failed to register codec: %d\n", ret);
		return -EIO;
	}

	*value = 0;
	for ( i = 1 ; i <= size ; ++i )
	{
		*value |= ( (uint)recv_buf[i-1] << ((size -i) << 3) );
	}

#ifdef SY24145A_DBG_REG_DUMP
	dev_dbg(dev, "I2C read address %d => %08x\n", size, *value);
#endif

	return 0;
}

static int sy24145a_reg_write(void *context, unsigned int reg,
			      unsigned int value)
{
	struct i2c_client *client = context;
	uint size, i;
	u8   buf[5];
	int  ret;
	struct device *dev = &client->dev;

	if ( reg > SY24145A_MAX_REGISTER )
		return -EINVAL;

	size = sy24145a_reg_size[reg];
	if (size == 0)
		return -EINVAL;
	if ( size > 4 )
		size = 4;

#ifdef SY24145A_DBG_REG_DUMP
	dev_dbg(dev, "I2C write address %d <= %08x\n", size, value);
#endif

	buf[0] = (u8)reg;
	for ( i = 1 ; i <= size ; ++i )
	{
		buf[i] = (u8)( value >> ((size - i) << 3) );
	}

	ret = i2c_master_send(client, buf, size + 1);
	if (ret == size + 1) {
		ret =  0;
	} else if (ret < 0) {
		dev_err(dev, "I2C write address failed\n");
	} else {
		dev_err(dev, "I2C write failed\n");
		ret =  -EIO;
	}

	return ret;
}


static struct regmap_config sy24145a_regmap_config = {
	.reg_bits = 8,
	.val_bits = 32,

	.max_register = SY24145A_MAX_REGISTER,
	.volatile_reg = sy24145a_reg_volatile,

	.cache_type = REGCACHE_RBTREE,
	.reg_defaults = sy24145a_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(sy24145a_reg_defaults),

	.reg_read  = sy24145a_reg_read,
	.reg_write = sy24145a_reg_write,
};


static int sy24145a_i2c_probe(struct i2c_client *i2c,
			const struct i2c_device_id *id)
{
	int ret;

	sy24145a_priv_data = devm_kzalloc(&i2c->dev, sizeof(struct sy24145a_priv), GFP_KERNEL);
	if (!sy24145a_priv_data)
		return -ENOMEM;

	sy24145a_priv_data->regmap = devm_regmap_init(&i2c->dev, NULL, i2c, &sy24145a_regmap_config);

	if (IS_ERR(sy24145a_priv_data->regmap)) {
		ret = PTR_ERR(sy24145a_priv_data->regmap);
		return ret;
	}

	mutex_init(&sy24145a_priv_data->dap_ram_lock);
	i2c_set_clientdata(i2c, sy24145a_priv_data);

	ret = snd_soc_register_codec(&i2c->dev,
				&soc_codec_dev_sy24145a, &sy24145a_dai, 1);
	if (ret != 0)
		dev_err(&i2c->dev, "Failed to register codec (%d)\n", ret);

	return ret;
}


static int sy24145a_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);

	return 0;
}


static const struct i2c_device_id sy24145a_i2c_id[] = {
	{ "sy24145a", 0 },
	{ }
};

struct snd_rpi_simple_drvdata {
	struct snd_soc_dai_link *dai;
	const char* card_name;
	unsigned int fixed_bclk_ratio;
};

static struct snd_soc_dai_link snd_sy24145a_amp_dai[] = {
	{
		.name = "SY24145A AMP",
		.stream_name = "SY24145A AMP HiFi",
		.codec_dai_name = "sy24145a-hifi",
		.codec_name = "sy24145a.5-002a",
		.dai_fmt = SND_SOC_DAIFMT_I2S |
					SND_SOC_DAIFMT_NB_NF |
					SND_SOC_DAIFMT_CBS_CFS,
	},
};

static struct snd_rpi_simple_drvdata drvdata_sy24145a_amp = {
	.card_name = "snd_rpi_sy24145a_amp",
	.dai = snd_sy24145a_amp_dai,
	.fixed_bclk_ratio = 64,
};

static const struct of_device_id sy24145a_i2c_match[] = {
	{.compatible = "silergy,sy24145a-amp",
		.data = (void *)&drvdata_sy24145a_amp },
	{ /* senitel */ }
};
MODULE_DEVICE_TABLE(of, sy24145a_i2c_match);



static struct i2c_driver sy24145a_i2c_driver = {
	.driver = {
		.name = "sy24145a",
		.owner = THIS_MODULE,
		.of_match_table = sy24145a_of_match,
	},
	.probe = sy24145a_i2c_probe,
	.remove = sy24145a_i2c_remove,
	.id_table = sy24145a_i2c_id
};

module_i2c_driver(sy24145a_i2c_driver);


MODULE_AUTHOR("XiaoTian Su <xiaotian.su@silergycorp.com>");
MODULE_DESCRIPTION("ASoC driver for SY24145A");
MODULE_LICENSE("GPL v2");
