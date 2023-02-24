/*
 * ASoC Driver Header File for SY24145A
*/
#ifndef _SY24145A_H_
#define _SY24145A_H_

//SY24145A I2C-bus register addresses
#define SY24145A_CLK_CTL_REG		0x00	// 1 byte
#define SY24145A_DEV_ID_REG			0x01	// 1 byte
#define SY24145A_ERR_STATUS_REG		0x02	// 1 byte
#define SY24145A_SYS_CTL_REG1		0x03	// 1 byte
#define SY24145A_SYS_CTL_REG2		0x04	// 1 byte
#define SY24145A_SYS_CTL_REG3		0x05	// 1 byte
#define SY24145A_SOFT_MUTE			0x06	// 1 byte
#define SY24145A_MAS_VOL			0x07	// 1 byte , Master volume
#define SY24145A_CH1_VOL			0x08	// 1 byte , Channel 1 vol
#define SY24145A_CH2_VOL			0x09	// 1 byte , Channel 2 vol
#define SY24145A_FINE_VOL			0x0B	// 1 byte , mvol_fine_tune
#define SY24145A_DC_SOFT_RESET		0x0F	// 1 byte
#define SY24145A_MOD_LMT_REG		0x10	// 1 byte
#define SY24145A_PWM_DLA_CH1P		0x11	// 1 byte
#define SY24145A_PWM_DLA_CH1N		0x12	// 1 byte
#define SY24145A_PWM_DLA_CH2P		0x13	// 1 byte
#define SY24145A_PWM_DLA_CH2N		0x14	// 1 byte
#define SY24145A_I2S_CONTROL		0x15	// 1 byte
#define SY24145A_DSP_CTL_REG1		0x16	// 1 byte
#define SY24145A_MONITOR_CFG1		0x17	// 1 byte
#define SY24145A_MONITOR_CFG2		0x18	// 1 byte
#define SY24145A_PWM_DC_THRESH		0x19	// 1 byte
#define SY24145A_HP_CTRL			0x1A	// 1 byte
#define SY24145A_OC_CTRL			0x1B	// 1 byte
#define SY24145A_OP_MODE			0x1E	// 1 byte
#define SY24145A_CHECKSUM_CTL		0x1F	// 1 byte
#define SY24145A_IN_MUX_REG			0x20	// 1 byte
#define SY24145A_DSP_CTL_REG2		0x21	// 1 byte
#define SY24145A_PWM_CTRL			0x22	// 1 byte
#define SY24145A_FAULT_SELECT		0x23	// 1 byte
#define SY24145A_CH1_EQ_CTRL1		0x25	// 1 byte
#define SY24145A_CH1_EQ_CTRL2		0x26	// 1 byte
#define SY24145A_CH2_EQ_CTRL1		0x27	// 1 byte
#define SY24145A_CH2_EQ_CTRL2		0x28	// 1 byte
#define SY24145A_SPEQ_CTRL_1		0x29	// 1 byte
#define SY24145A_SPEQ_CTRL_2		0x2A	// 1 byte
#define SY24145A_SPEQ_CTRL_3		0x2B	// 1 byte
#define SY24145A_PRE_SCALER			0x2C	// 2 byte , Prescaler
#define SY24145A_POST_SCALER		0x2D	// 2 byte , postscaler
#define SY24145A_CH12_BQ0			0x2F	// 20 byte , BQ0 coefficient
#define SY24145A_CH12_BQ1			0x30	// 20 byte , BQ1 coefficient
#define SY24145A_CH12_BQ2			0x31	// 20 byte , BQ2 coefficient
#define SY24145A_CH12_BQ3			0x32	// 20 byte , BQ3 coefficient
#define SY24145A_CH12_BQ4			0x33	// 20 byte , BQ4 coefficient
#define SY24145A_CH12_BQ5			0x34	// 20 byte , BQ5 coefficient
#define SY24145A_CH12_BQ6			0x35	// 20 byte , BQ6 coefficient
#define SY24145A_CH12_BQ7			0x36	// 20 byte , BQ7 coefficient
#define SY24145A_CH12_BQ8			0x37	// 20 byte , BQ8 coefficient
#define SY24145A_CH12_BQ9			0x38	// 20 byte , BQ9 coefficient
#define SY24145A_CH12_BQ10			0x39	// 20 byte , BQ10 coefficient
#define SY24145A_CH12_BQ11			0x3A	// 20 byte , BQ11 coefficient
#define SY24145A_CH12_BQ12			0x3B	// 20 byte , BQ12 coefficient
#define SY24145A_CH12_BQ13			0x3C	// 20 byte , BQ13 coefficient
#define SY24145A_CH12_BQ14			0x3D	// 20 byte , BQ14 coefficient
#define SY24145A_CH12_BQ15			0x3E	// 20 byte , BQ15 coefficient
#define SY24145A_CH12_BQ16			0x3F	// 20 byte , BQ16 coefficient
#define SY24145A_CH12_BQ17			0x40	// 20 byte , BQ17 coefficient
#define SY24145A_CH12_COEF_SPEQ0	0x41	// 12 byte , SPEQ0 coefficient
#define SY24145A_CH12_COEF_SPEQ1	0x42	// 12 byte , SPEQ1 coefficient
#define SY24145A_CH12_COEF_SPEQ2	0x43	// 12 byte , SPEQ2 coefficient
#define SY24145A_CH12_COEF_SPEQ3	0x44	// 12 byte , SPEQ3 coefficient
#define SY24145A_CH12_COEF_SPEQ4	0x45	// 12 byte , SPEQ4 coefficient
#define SY24145A_CH12_COEF_SPEQ5	0x46	// 12 byte , SPEQ5 coefficient
#define SY24145A_DRC_BQ0			0x47	// 20 byte , DRC_BQ0 coefficient
#define SY24145A_DRC_BQ1			0x48	// 20 byte , DRC_BQ1 coefficient
#define SY24145A_DRC_BQ2			0x49	// 20 byte , DRC_BQ2 coefficient
#define SY24145A_DRC_BQ3			0x4A	// 20 byte , DRC_BQ3 coefficient
#define SY24145A_DRC_BQ4			0x4B	// 20 byte , DRC_BQ4 coefficient
#define SY24145A_DRC_BQ5			0x4C	// 20 byte , DRC_BQ5 coefficient
#define SY24145A_DRC_BQ6			0x4D	// 20 byte , DRC_BQ6 coefficient
#define SY24145A_DRC_BQ7			0x4E	// 20 byte , DRC_BQ7 coefficient
#define SY24145A_DRC_BQ8			0x4F	// 20 byte , DRC_BQ8 coefficient
#define SY24145A_DRC_BQ9			0x50	// 20 byte , DRC_BQ9 coefficient
#define SY24145A_DRC_BQ10			0x51	// 20 byte , DRC_BQ10 coefficient
#define SY24145A_DRC_BQ11			0x52	// 20 byte , DRC_BQ11 coefficient
#define SY24145A_DRC_BQ12			0x53	// 20 byte , DRC_BQ12 coefficient
#define SY24145A_DRC_BQ13			0x54	// 20 byte , DRC_BQ13 coefficient
#define SY24145A_DRC_BQ14			0x55	// 20 byte , DRC_BQ14 coefficient
#define SY24145A_DRC_BQ15			0x56	// 20 byte , DRC_BQ15 coefficient
#define SY24145A_LOUDNESS_LR		0x57	// 12 byte , Loudness gain
#define SY24145A_SPEQ_ATK_REL_TC_1	0x5D	// 18 byte , speq_atk_rel_tc_1
#define SY24145A_SPEQ_ATK_REL_TC_2	0x5E	// 18 byte , speq_atk_rel_tc_2
#define SY24145A_MIXER_CTRL			0x5F	// 4 byte , ch12 mixer gain
#define SY24145A_DRC_CTRL			0x60	// 4 byte , DRC control
#define SY24145A_LDRC_LMT_CFG1		0x61	// 3 byte , LDRC makeup gain and threshold
#define SY24145A_LDRC_LMT_CFG2		0x62	// 3 byte , LDRC attack time control
#define SY24145A_LDRC_LMT_CFG3		0x63	// 3 byte , LDRC release time control
#define SY24145A_MDRC_LMT_CFG1		0x64	// 3 byte , MDRC makeup gain and threshold
#define SY24145A_MDRC_LMT_CFG2		0x65	// 3 byte , MDRC attack time control
#define SY24145A_MDRC_LMT_CFG3		0x66	// 3 byte , LDRC release time control
#define SY24145A_HDRC_LMT_CFG1		0x67	// 3 byte , HDRC makeup gain and threshold
#define SY24145A_HDRC_LMT_CFG2		0x68	// 3 byte , HDRC attack time control
#define SY24145A_HDRC_LMT_CFG3		0x69	// 3 byte , LDRC release time control
#define SY24145A_PDRC_LMT_CFG1		0x6A	// 3 byte , PDRC threshold
#define SY24145A_PDRC_LMT_CFG2		0x6B	// 3 byte , PDRC attack time control
#define SY24145A_PDRC_LMT_CFG3		0x6C	// 3 byte , PDRC release time control
#define SY24145A_PDRC_ENVLP_TC_UP	0x6D	// 3 byte , the attack coefficient of PDRC signal level envelop detection
#define SY24145A_PDRC_ENVLP_TC_DN	0x6E	// 3 byte , the release coefficient of PDRC signal level envelop detection
#define SY24145A_PROT_SYS_CNTL		0x76	// 1 byte
#define SY24145A_HARD_CLIPPER_THR	0x78	// 3 byte
#define SY24145A_DRC_GAIN_OFFSET	0x79	// 4 byte
#define SY24145A_INTER_PRIVATE		0x82	// 4 byte
#define SY24145A_OCDET_WIND_WIDTH	0x85	// 4 byte
#define SY24145A_FAULT_OC_TH		0x86	// 4 byte
#define SY24145A_DSP_CTL3			0x8B	// 1 byte
#define SY24145A_FUNC_DEBUG			0x8C	// 1 byte
#define SY24145A_LDRC_ENVLP_TC_UP	0x8E	// 3 byte , the attack coefficient of LDRC signal level envelop detection
#define SY24145A_LDRC_ENVLP_TC_DN	0x8F	// 3 byte , the release coefficient of LDRC signal level envelop detection
#define SY24145A_MDRC_ENVLP_TC_UP	0x90	// 3 byte , the attack coefficient of MDRC signal level envelop detection
#define SY24145A_MDRC_ENVLP_TC_DN	0x91	// 3 byte , the release coefficient of MDRC signal level envelop detection
#define SY24145A_HDRC_ENVLP_TC_UP	0x92	// 3 byte , the attack coefficient of HDRC signal level envelop detection
#define SY24145A_HDRC_ENVLP_TC_DN	0x93	// 3 byte , the release coefficient of HDRC signal level envelop detection
#define SY24145A_PWM_MUX_REG		0x94	// 4 byte , PWM MUX register
#define SY24145A_PWM_OUTFLIP		0x95	// 4 byte , PWM Outflip register
#define SY24145A_PWM_CTL_REG		0x96	// 4 byte , PWM Control register
#define SY24145A_PM_COEF			0x97	// 6 byte , power meter envelop detection attack and release coefficient
#define SY24145A_PM_CTL_RB1			0x98	// 3 byte , power meter control_1 
#define SY24145A_PM_CTL_RB2			0x99	// 3 byte , power meter control_2 
#define SY24145A_CH1_PBQ_CHECKSUM	0x9A	// 4 byte
#define SY24145A_DRC_CHECKSUM 		0x9B	// 4 byte
#define SY24145A_CH2_PBQ_CHECKSUM	0x9C	// 4 byte
#define SY24145A_DSP_DEBUG_REG1		0xB0	// 4 byte

#define SY24145A_MAX_REGISTER            0xFF

// 0x03: SY24145A_SYS_CTL_REG1 : 1 byte
#define SY24145A_I2C_ACCESS_RAM_MASK     BIT(0)
#define SY24145A_I2C_ACCESS_RAM_ENABLE   BIT(0)
#define SY24145A_I2C_ACCESS_RAM_DISABLE  0

// 0x04: SY24145A_SYS_CTL_REG2 : 1 byte
#define SY24145A_EQ_ENABLE_SHIFT         (4)

// 0x06: SY24145A_SOFT_MUTE    : 1 byte
#define SY24145A_SOFT_MUTE_ALL           (BIT(3)|BIT(1)|BIT(0))
#define SY24145A_SOFT_MUTE_MASTER_SHIFT  (3)
#define SY24145A_SOFT_MUTE_CH1_SHIFT     (0)
#define SY24145A_SOFT_MUTE_CH2_SHIFT     (1)

// 0x60: SY24145A_DRC_CTRL     : 4 byte
#define SY24145A_DRC_EN_MASK             (BIT(3)|BIT(2)|BIT(1)|BIT(0))
#define SY24145A_DRC_ENABLE              (BIT(3)|BIT(2)|BIT(1)|BIT(0))
#define SY24145A_DRC_DISABLE             0

#endif  /* _SY24145A_H_ */