#include "sy24145a_reg.h"

enum SY24145A_DAP_RAM_ITEM {
	DAP_RAM_EQ_BIQUAD_0	= 0,
	DAP_RAM_EQ_BIQUAD_1	= 1,
	DAP_RAM_EQ_BIQUAD_2	= 2,
	DAP_RAM_EQ_BIQUAD_3	= 3,
	DAP_RAM_EQ_BIQUAD_4	= 4,
	DAP_RAM_EQ_BIQUAD_5	= 5,
	DAP_RAM_EQ_BIQUAD_6	= 6,
	DAP_RAM_EQ_BIQUAD_7	= 7,
	DAP_RAM_EQ_BIQUAD_8	= 8,
	DAP_RAM_EQ_BIQUAD_9	= 9,
	DAP_RAM_EQ_BIQUAD_10	= 10,
	DAP_RAM_EQ_BIQUAD_11	= 11,
	DAP_RAM_EQ_BIQUAD_12	= 12,
	DAP_RAM_EQ_BIQUAD_13	= 13,
	DAP_RAM_EQ_BIQUAD_14	= 14,
	DAP_RAM_EQ_BIQUAD_15	= 15,
	DAP_RAM_EQ_BIQUAD_16	= 16,
	DAP_RAM_EQ_BIQUAD_17	= 17,
	DAP_RAM_SPEQ_COEF_0	= 18,
	DAP_RAM_SPEQ_COEF_1	= 19,
	DAP_RAM_SPEQ_COEF_2	= 20,
	DAP_RAM_SPEQ_COEF_3	= 21,
	DAP_RAM_SPEQ_COEF_4	= 22,
	DAP_RAM_SPEQ_COEF_5	= 23,
	DAP_RAM_DRC_BIQUAD_0	= 24,
	DAP_RAM_DRC_BIQUAD_1	= 25,
	DAP_RAM_DRC_BIQUAD_2	= 26,
	DAP_RAM_DRC_BIQUAD_3	= 27,
	DAP_RAM_DRC_BIQUAD_4	= 28,
	DAP_RAM_DRC_BIQUAD_5	= 29,
	DAP_RAM_DRC_BIQUAD_6	= 30,
	DAP_RAM_DRC_BIQUAD_7	= 31,
	DAP_RAM_DRC_BIQUAD_8	= 32,
	DAP_RAM_DRC_BIQUAD_9	= 33,
	DAP_RAM_DRC_BIQUAD_10	= 34,
	DAP_RAM_DRC_BIQUAD_11	= 35,
	DAP_RAM_DRC_BIQUAD_12	= 36,
	DAP_RAM_DRC_BIQUAD_13	= 37,
	DAP_RAM_DRC_BIQUAD_14	= 38,
	DAP_RAM_DRC_BIQUAD_15	= 39,
	DAP_RAM_LOUDNESS_GAIN  = 40,
	DAP_RAM_ITEM_NUM       = 41,
};

struct sy24145a_dap_ram_command {
	const enum SY24145A_DAP_RAM_ITEM item;
	const unsigned char *const data;
};

struct sy24145a_reg {
	uint reg;
	uint val;
};

//Register Table of SY24145A
static const u8 sy24145a_reg_size[SY24145A_MAX_REGISTER + 1] =
{//  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // 0
	 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // 1
	 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  1, 20, // 2
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, // 3
	20, 12, 12, 12, 12, 12, 12, 20, 20, 20, 20, 20, 20, 20, 20, 20, // 4
	20, 20, 20, 20, 20, 20, 20, 12,  1,  1,  1,  1,  1, 18, 18,  4, // 5
	 4,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, // 6
	 1,  4,  2,  1,  1,  1,  1,  1,  3,  4,  1,  1,  4,  4,  4,  1, // 7
	 3,  3,  4,  1,  1,  4,  4,  1,  1,  1,  1,  1,  1,  1,  3,  3, // 8
	 3,  3,  3,  3,  4,  4,  4,  6,  3,  3,  4,  4,  4,  3,  3,  3, // 9
	 3,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // A
	 4                                                              // B
};

static const struct sy24145a_dap_ram_command sy24145a_dap_ram_init[] =
{
	{.item = DAP_RAM_SPEQ_COEF_0,.data = "\x41\x02\x00\x00\x00\x04\x00\x00\x00\x00\x20\x00\x00" },
	{.item = DAP_RAM_SPEQ_COEF_1,.data = "\x42\x02\x00\x00\x00\x04\x00\x00\x00\x00\x20\x00\x00" },
	{.item = DAP_RAM_SPEQ_COEF_2,.data = "\x43\x02\x00\x00\x00\x04\x00\x00\x00\x00\x20\x00\x00" },
	{.item = DAP_RAM_SPEQ_COEF_3,.data = "\x44\x02\x00\x00\x00\x04\x00\x00\x00\x00\x20\x00\x00" },
	{.item = DAP_RAM_SPEQ_COEF_4,.data = "\x45\x02\x00\x00\x00\x04\x00\x00\x00\x00\x20\x00\x00" },
	{.item = DAP_RAM_SPEQ_COEF_5,.data = "\x46\x02\x00\x00\x00\x04\x00\x00\x00\x00\x20\x00\x00" },

	{.item = DAP_RAM_DRC_BIQUAD_0,.data = "\x47\x00\x00\x00\x00\x02\xAC\x37\x05\x00\x00\x00\x00\x00\xA9\xE4\x7D\x00\xA9\xE4\x7D" },
	{.item = DAP_RAM_DRC_BIQUAD_1,.data = "\x48\xFD\x48\xEE\x0A\x06\x34\x36\xB8\x00\x20\xB6\xD0\x00\x41\x6D\x9F\x00\x20\xB6\xD0" },
	{.item = DAP_RAM_DRC_BIQUAD_2,.data = "\x49\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_DRC_BIQUAD_3,.data = "\x4A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_DRC_BIQUAD_4,.data = "\x4B\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_DRC_BIQUAD_5,.data = "\x4C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_DRC_BIQUAD_6,.data = "\x4D\x00\x00\x00\x00\x02\xAC\x37\x05\x00\x00\x00\x00\xFC\xA9\xE4\x7E\x03\x56\x1B\x83" },
	{.item = DAP_RAM_DRC_BIQUAD_7,.data = "\x4E\xFD\x48\xEE\x0A\x06\x34\x36\xB8\x03\x3A\xD2\x2B\xF9\x8A\x5B\xAA\x03\x3A\xD2\x2B" },
	{.item = DAP_RAM_DRC_BIQUAD_8,.data = "\x4F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_DRC_BIQUAD_9,.data = "\x50\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_DRC_BIQUAD_10,.data = "\x51\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_DRC_BIQUAD_11,.data = "\x52\x00\x00\x00\x00\x03\xD8\x8E\x98\x00\x00\x00\x00\x00\x13\xB8\xB4\x00\x13\xB8\xB4" },
	{.item = DAP_RAM_DRC_BIQUAD_12,.data = "\x53\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_DRC_BIQUAD_13,.data = "\x54\x00\x00\x00\x00\x03\xD8\x8E\x98\x00\x00\x00\x00\xFC\x13\xB8\xB5\x03\xEC\x47\x4C" },
	{.item = DAP_RAM_DRC_BIQUAD_14,.data = "\x55\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_DRC_BIQUAD_15,.data = "\x56\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },

	{.item = DAP_RAM_LOUDNESS_GAIN,.data = "\x57\x40\x00\x00\x00\x40\x00\x00\x00\x40\x00\x00\x00" },
};

static const struct sy24145a_reg sy24145a_reg_init_mute[] =
{
	{.reg = SY24145A_CLK_CTL_REG,	.val= 0x1A },
	{.reg = SY24145A_I2S_CONTROL,	.val= 0x10 },
	{.reg = SY24145A_SOFT_MUTE,	.val= 0x08 },

};

static const struct sy24145a_dap_ram_command sy24145a_dap_ram_eq_1[] =
{
	{.item = DAP_RAM_EQ_BIQUAD_0,.data = "\x2F\xFD\x3F\x8A\x9B\x06\x85\x90\x12\x00\x0E\xB9\x55\x00\x1D\x72\xA9\x00\x0E\xB9\x55" },
	{.item = DAP_RAM_EQ_BIQUAD_1,.data = "\x30\xFD\x3F\x8A\x9B\x06\x85\x90\x12\x00\x0E\xB9\x55\x00\x1D\x72\xA9\x00\x0E\xB9\x55" },
	{.item = DAP_RAM_EQ_BIQUAD_2,.data = "\x31\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_3,.data = "\x32\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_4,.data = "\x33\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_5,.data = "\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_6,.data = "\x35\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_7,.data = "\x36\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_8,.data = "\x37\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_9,.data = "\x38\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_10,.data = "\x39\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_11,.data = "\x3A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_12,.data = "\x3B\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_13,.data = "\x3C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_14,.data = "\x3D\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_15,.data = "\x3E\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_16,.data = "\x3F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },
	{.item = DAP_RAM_EQ_BIQUAD_17,.data = "\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00" },

};

static const struct sy24145a_reg sy24145a_reg_eq_1[]= {
	{ .reg = SY24145A_CH1_EQ_CTRL1,		.val = 0xFF		}, //CH1_EQ_CTRL1
	{ .reg = SY24145A_CH1_EQ_CTRL2,		.val = 0xFF		}, //CH1_EQ_CTRL2
	{ .reg = SY24145A_CH2_EQ_CTRL1,		.val = 0xFF		}, //CH2_EQ_CTRL1
	{ .reg = SY24145A_CH2_EQ_CTRL2,		.val = 0xFF		}, //CH2_EQ_CTRL2
};

static const struct sy24145a_reg sy24145a_reg_speq_ctrl[]= {
	{ .reg = SY24145A_SPEQ_CTRL_1,		.val = 0xF0		}, //SPEQ_CTRL_1
	{ .reg = SY24145A_SPEQ_CTRL_2,		.val = 0x00		}, //SPEQ_CTRL_2
	{ .reg = SY24145A_SPEQ_CTRL_3,		.val = 0x00		}, //SPEQ_CTRL_3
};

static const struct sy24145a_reg sy24145a_reg_init_seq_0[] =
{
	{.reg = SY24145A_CLK_CTL_REG,   .val= 0x0A },
	{.reg = SY24145A_OC_CTRL,	.val= 0xBD },
	{.reg = SY24145A_DSP_CTL_REG2,	.val= 0x00 },
	{.reg = SY24145A_FAULT_SELECT,	.val= 0x1A },
	{.reg = SY24145A_SYS_CTL_REG3,	.val= 0x02 },
	{.reg = SY24145A_PROT_SYS_CNTL,	.val= 0x1F },
	{.reg = SY24145A_HARD_CLIPPER_THR,	.val= 0x7FFFFF },
	{.reg = SY24145A_INTER_PRIVATE,	.val= 0x000000F0 },
	{.reg = SY24145A_OCDET_WIND_WIDTH,	.val= 0x00000006 },
	{.reg = SY24145A_FAULT_OC_TH,	.val= 0x00003010 },
	{.reg = SY24145A_PWM_MUX_REG,	.val= 0x00000010 },
	{.reg = SY24145A_PWM_OUTFLIP,	.val= 0x40003210 },
	{.reg = SY24145A_PWM_CTL_REG,	.val= 0x10000038 },
	{.reg = SY24145A_DSP_DEBUG_REG1,	.val= 0x000007FF },
	{.reg = SY24145A_PWM_DC_THRESH,	.val= 0x15 },
	{.reg = SY24145A_MOD_LMT_REG,	.val= 0x77 },
	{.reg = SY24145A_PWM_DLA_CH1P,	.val= 0x00 },
	{.reg = SY24145A_PWM_DLA_CH1N,	.val= 0x00 },
	{.reg = SY24145A_PWM_DLA_CH2P,	.val= 0x06 },
	{.reg = SY24145A_PWM_DLA_CH2N,	.val= 0x06 },

};

static const struct sy24145a_reg sy24145a_reg_init_seq_1[] =
{
	{.reg = SY24145A_LDRC_ENVLP_TC_UP,	.val= 0x010000 },
	{.reg = SY24145A_LDRC_ENVLP_TC_DN,	.val= 0x7FD000 },
	{.reg = SY24145A_MDRC_ENVLP_TC_UP,	.val= 0x010000 },
	{.reg = SY24145A_MDRC_ENVLP_TC_DN,	.val= 0x700000 },
	{.reg = SY24145A_HDRC_ENVLP_TC_UP,	.val= 0x010000 },
	{.reg = SY24145A_HDRC_ENVLP_TC_DN,	.val= 0x700000 },
	{.reg = SY24145A_PDRC_ENVLP_TC_UP,	.val= 0x010000 },
	{.reg = SY24145A_PDRC_ENVLP_TC_DN,	.val= 0x700000 },

	{.reg = SY24145A_LDRC_LMT_CFG1,	.val= 0x3CC30C },
	{.reg = SY24145A_LDRC_LMT_CFG2,	.val= 0x00F3CF },
	{.reg = SY24145A_LDRC_LMT_CFG3,	.val= 0x000123 },
	{.reg = SY24145A_MDRC_LMT_CFG1,	.val= 0x3CC30C },
	{.reg = SY24145A_MDRC_LMT_CFG2,	.val= 0x00F3CF },
	{.reg = SY24145A_MDRC_LMT_CFG3,	.val= 0x000123 },
	{.reg = SY24145A_HDRC_LMT_CFG1,	.val= 0x3CC30C },
	{.reg = SY24145A_HDRC_LMT_CFG2,	.val= 0x00F3CF },
	{.reg = SY24145A_HDRC_LMT_CFG3,	.val= 0x000123 },
	{.reg = SY24145A_PDRC_LMT_CFG1,	.val= 0x00030C },
	{.reg = SY24145A_PDRC_LMT_CFG2,	.val= 0x00F3CF },
	{.reg = SY24145A_PDRC_LMT_CFG3,	.val= 0x000123 },

	{.reg = SY24145A_DRC_GAIN_OFFSET,	.val= 0x000000F0 },
	{.reg = SY24145A_DSP_CTL3,	.val= 0x30 },
	{.reg = SY24145A_FUNC_DEBUG,	.val= 0x18 },
	{.reg = SY24145A_CH1_PBQ_CHECKSUM,	.val= 0xF0AFDA3C },
	{.reg = SY24145A_DRC_CHECKSUM ,	.val= 0xB9CB3631 },
	{.reg = SY24145A_CH2_PBQ_CHECKSUM,	.val= 0xF0AFDA3C },

	{.reg = SY24145A_DRC_CTRL,	.val= 0x0000080F },
	{.reg = SY24145A_OP_MODE,	.val= 0x07 },
	{.reg = SY24145A_MAS_VOL,	.val= 0xFF },
	{.reg = SY24145A_CH1_VOL,	.val= 0x9F },
	{.reg = SY24145A_CH2_VOL,	.val= 0x9F },
	{.reg = SY24145A_FINE_VOL,	.val= 0x00 },
	{.reg = SY24145A_IN_MUX_REG,	.val= 0x80 },
	{.reg = SY24145A_DSP_CTL_REG1,	.val= 0x06 },
	{.reg = SY24145A_MONITOR_CFG1,	.val= 0x85 },
	{.reg = SY24145A_MONITOR_CFG2,	.val= 0x00 },
	{.reg = SY24145A_PRE_SCALER,	.val= 0x7FFF },
	{.reg = SY24145A_POST_SCALER,	.val= 0x75CF },
	{.reg = SY24145A_CHECKSUM_CTL,	.val= 0x03 },
	{.reg = SY24145A_PWM_CTRL,	.val= 0x00 },
	{.reg = SY24145A_SOFT_MUTE,	.val= 0x00 },

};

