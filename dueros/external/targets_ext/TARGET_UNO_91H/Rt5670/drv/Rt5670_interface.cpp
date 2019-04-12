#include "mbed.h"
#include "mbed_interface.h"
#include "rt5670_interface.h"
#include "rda_i2s_api.h"
#include "I2C.h"
//#include "I2C.h"


#define LOG_5670(_fmt, ...)  printf("[RT5670] ==> %s(%d): "_fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)
typedef unsigned short u16;

void wait_1us(int time)
{
	int i = 0; 
	int bound = time * 100;

	for(i = 0; i < bound ; ++i);
}

/**
 * @brief Inite i2s master mode
 *
 * @return
 *     - (-1)  Error
 *     - (0)  Success
 */
i2s_t i2s_obj = {0};
i2s_cfg_t i2s_cfg;

int I2sInit(void)
{
    i2s_cfg.mode              = I2S_MD_MASTER_RX;
    i2s_cfg.rx.fs             = I2S_64FS;
    i2s_cfg.rx.ws_polarity    = I2S_WS_NEG;
    i2s_cfg.rx.std_mode       = I2S_STD_M;
    i2s_cfg.rx.justified_mode = I2S_RIGHT_JM;
    i2s_cfg.rx.data_len       = I2S_DL_16b;
    i2s_cfg.rx.msb_lsb        = I2S_MSB;

    i2s_cfg.tx.fs             = I2S_64FS;
    i2s_cfg.tx.ws_polarity    = I2S_WS_NEG;
    i2s_cfg.tx.std_mode       = I2S_STD_M;
    i2s_cfg.tx.justified_mode = I2S_RIGHT_JM;
    i2s_cfg.tx.data_len       = I2S_DL_16b;
    i2s_cfg.tx.msb_lsb        = I2S_MSB;
    i2s_cfg.tx.wrfifo_cntleft = I2S_WF_CNTLFT_8W;

    //rda_i2s_init(&i2s_obj, I2S_TX_BCLK, I2S_TX_WS, I2S_TX_SD, NC, NC, I2S_RX_SD, GPIO_PIN5);
	rda_i2s_init(&i2s_obj, I2S_TX_BCLK, I2S_TX_WS, I2S_TX_SD, NC, NC, I2S_RX_SD, NC);
    rda_i2s_set_ws(&i2s_obj, 16000, 768);
    rda_i2s_set_tx_channel(&i2s_obj, 1);
    rda_i2s_set_rx_channel(&i2s_obj, 1);
    rda_i2s_format(&i2s_obj, &i2s_cfg);

    //rda_i2s_enable_rx(&i2s_obj);
    //rda_i2s_enable_tx(&obj);

	return 0;
}

void I2sRx_Open()
{
	rda_i2s_enable_master_rx();


	rda_i2s_enable_rx(&i2s_obj);
	rda_i2s_enable_tx(&i2s_obj);

	rda_i2s_out_mute(&i2s_obj);
}

void I2sTx_Open()
{
	rda_i2s_enable_tx(&i2s_obj);
}

void I2sRx_Close()
{
	rda_i2s_disable_rx(&i2s_obj);
}

void I2sTx_Close()
{
	rda_i2s_disable_tx(&i2s_obj);
}

int I2s_Recv_Data(void* data , int len)
{
    rda_i2s_int_recv(&i2s_obj, (uint32_t *)data , len/sizeof(uint32_t));
	return len;
}

int I2s_Send_Data(void* data , int len)
{
    rda_i2s_int_send(&i2s_obj, (uint32_t *)data , len/sizeof(uint32_t));	
	return len;
}

int I2s_Wait_TxDone()
{
	if (I2S_ST_BUSY == i2s_obj.sw_tx.state) {
		rda_i2s_sem_wait(i2s_tx_sem, osWaitForever);
	}

	return 0;
}

int I2s_Wait_RxDone()
{
	if (I2S_ST_BUSY == i2s_obj.sw_rx.state) {
		rda_i2s_sem_wait(i2s_rx_sem, osWaitForever);
	}

	return 0;
}


#define SDA_PIN  GPIO_PIN22                         
#define SCL_PIN  GPIO_PIN23                         

I2C *i2c;

typedef unsigned char uint8;
typedef unsigned short uint16;

void FM36Iint(void);
void DspTuneParaSetNS(void);
void DspTuneParaSetAEC(void);

void DspSetPath(void);
void delay()
{
	int i;	
	for(i=0; i <100; ++i);//100--->5us
}

#define ALC5670_I2C_ADDR_W 0X38	
#define ALC5670_I2C_ADDR_R 0X39		

int rt5670WriteReg (unsigned char Reg, unsigned short Value)  
{ 
    unsigned char txbuf[4] = {0, 0, 0, 0};    
	unsigned char rxbuf[4] = {0, 0, 0, 0};  

	unsigned char highByte;
	unsigned char lowByte;
    //S +  DEV_ADDR+WR  + ACK +  REG_ADDR  + ACK +  DATA_H  + ACK +  DATA_L  + ACK  + P
	highByte=(unsigned char)((Value>>8)&0xFF);
	lowByte=(unsigned char)((Value>>0)&0xFF);
	
    txbuf[0]=Reg;
	txbuf[1]=highByte;
	txbuf[2]=lowByte;
	
    i2c->write(ALC5670_I2C_ADDR_W,(char *)txbuf,3,false);
	 
    return 1;	
}

unsigned short rt5670ReadReg (unsigned char Reg,uint16_t *pData)  
{ 
    unsigned char txbuf[4] = {0, 0, 0, 0};    
	unsigned char rxbuf[4] = {0, 0, 0, 0};  

	unsigned char highByte=0;
	unsigned char lowByte=0;
	unsigned short Value=0;

    //S +  DEV_ADDR+WR  + ACK +  REG_ADDR  + ACK + S +  DEV_ADDR+RD  + ACK + DATA_H  + MACK +  DATA_L  + MNACK  + P
	txbuf[0]=Reg;
	i2c->write(ALC5670_I2C_ADDR_W,(char *)txbuf,1,true);
	i2c->read(ALC5670_I2C_ADDR_R, (char *)rxbuf, 2, false);
	highByte=rxbuf[0];
	lowByte=rxbuf[1];

    *pData=((unsigned short)highByte<<8)|lowByte;
	return Value;
}

int rt5670_update_bits(uint8_t reg, uint16_t mask, uint16_t value)
{
	uint16_t data;
	rt5670ReadReg(reg, &data);

	data &= ~mask;
	data |= value;

	return rt5670WriteReg(reg, data);
}

static int rt5670_index_write(	uint8_t reg, uint16_t value)
{
	int ret = 0;

	ret |= rt5670WriteReg(RT5670_PRIV_INDEX, reg);
	ret |= rt5670WriteReg(RT5670_PRIV_DATA, value);

	return ret;
}

static unsigned int rt5670_index_read(uint8_t reg, uint16_t *pData)
{
	int ret = 0;

	ret |= rt5670WriteReg(RT5670_PRIV_INDEX, reg);
	ret |= rt5670ReadReg(RT5670_PRIV_DATA, pData);

	return ret;
}


/**
 * rt5670_dsp_done - Wait until DSP is ready.
 */
static int rt5670_dsp_done(void)
{
	unsigned int count = 0;
	uint16_t dsp_val;
	int ret = 0;

	ret = rt5670ReadReg(RT5670_DSP_CTRL1, &dsp_val);
	while (dsp_val & RT5670_DSP_BUSY_MASK) {
		if (count > 1000)
			return -99;
		ret = rt5670ReadReg(RT5670_DSP_CTRL1, &dsp_val);
		count++;
	}

	return 0;
}



int rt5670_dsp_write(uint16_t addr, uint16_t data)
{
	unsigned int dsp_val;
	int ret = 0;

	ret = rt5670WriteReg(RT5670_DSP_CTRL2, addr);
	if (ret < 0) {
		//LOG_5670("Failed to write DSP addr reg: %d\n", ret);
		goto err;
	}
	ret = rt5670WriteReg(RT5670_DSP_CTRL3, data);
	if (ret < 0) {
		//LOG_5670("Failed to write DSP data reg: %d\n", ret);
		goto err;
	}
	dsp_val = RT5670_DSP_I2C_AL_16 | RT5670_DSP_DL_2 |
		RT5670_DSP_CMD_MW | DSP_CLK_RATE | RT5670_DSP_CMD_EN;

	ret = rt5670WriteReg(RT5670_DSP_CTRL1, dsp_val);
	if (ret < 0) {
		//LOG_5670("Failed to write DSP cmd reg: %d\n", ret);
		goto err;
	}
	ret = rt5670_dsp_done();
	if (ret < 0) {
		//LOG_5670("DSP is busy: %d\n", ret);
		goto err;
	}

	return 0;

err:
	return ret;
}

/**
 * rt5670_dsp_read - Read DSP register.
 */
int rt5670_dsp_read(uint16_t reg, uint16_t *pData)
{
	uint16_t value;
	unsigned int dsp_val;
	int ret = 0;

	ret = rt5670_dsp_done();
	if (ret < 0) {
		//LOG_5670("DSP is busy: %d\n", ret);
		goto err;
	}

	ret = rt5670WriteReg(RT5670_DSP_CTRL2, reg);
	if (ret < 0) {
		//LOG_5670("Failed to write DSP addr reg: %d\n", ret);
		goto err;
	}
	dsp_val = RT5670_DSP_I2C_AL_16 | RT5670_DSP_DL_0 | RT5670_DSP_RW_MASK |
		RT5670_DSP_CMD_MR | DSP_CLK_RATE | RT5670_DSP_CMD_EN;

	ret = rt5670WriteReg(RT5670_DSP_CTRL1, dsp_val);
	if (ret < 0) {
		//LOG_5670("Failed to write DSP cmd reg: %d\n", ret);
		goto err;
	}

	ret = rt5670_dsp_done();
	if (ret < 0) {
		//LOG_5670("DSP is busy: %d\n", ret);
		goto err;
	}

	ret = rt5670WriteReg(RT5670_DSP_CTRL2, 0x26);
	if (ret < 0) {
		//LOG_5670("Failed to write DSP addr reg: %d\n", ret);
		goto err;
	}
	dsp_val = RT5670_DSP_DL_1 | RT5670_DSP_CMD_RR | RT5670_DSP_RW_MASK |
		DSP_CLK_RATE | RT5670_DSP_CMD_EN;

	ret = rt5670WriteReg(RT5670_DSP_CTRL1, dsp_val);
	if (ret < 0) {
		//LOG_5670("Failed to write DSP cmd reg: %d\n", ret);
		goto err;
	}

	ret = rt5670_dsp_done();
	if (ret < 0) {
		//LOG_5670("DSP is busy: %d\n", ret);
		goto err;
	}

	ret = rt5670WriteReg(RT5670_DSP_CTRL2, 0x25);
	if (ret < 0) {
		//LOG_5670("Failed to write DSP addr reg: %d\n", ret);
		goto err;
	}

	dsp_val = RT5670_DSP_DL_1 | RT5670_DSP_CMD_RR | RT5670_DSP_RW_MASK |
		DSP_CLK_RATE | RT5670_DSP_CMD_EN;

	ret = rt5670WriteReg(RT5670_DSP_CTRL1, dsp_val);
	if (ret < 0) {
		//LOG_5670("Failed to write DSP cmd reg: %d\n", ret);
		goto err;
	}

	ret = rt5670_dsp_done();
	if (ret < 0) {
		//LOG_5670("DSP is busy: %d\n", ret);
		goto err;
	}

	ret = rt5670ReadReg(RT5670_DSP_CTRL5, pData);
	if (ret < 0) {
		//LOG_5670("Failed to read DSP data reg: %d\n", ret);
		goto err;
	}

	//value = ret;
	//return value;

err:
	return ret;
}

/**
 * @brief Configure RT5670 ADC and DAC volume. Basicly you can consider this as ADC and DAC gain
 *
 * @param mode:             set ADC or DAC or all
 * @param volume:           0 ~ 127              0   : -17.625dB
 *												 47  : 0dB
 *												 127 : +30 dB, with 0.375dB/step
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int rt5670SetAdcDacVolume(int mode, int volume)
{
    int res = 0;
    if ( volume < 0 || volume > 127 ) {
        //LOG_5670("Warning: volume < 0! or > 127!\n");
        if (volume < 0)
            volume = 0;
        else
            volume = 127;
    }

    if (mode == RT_MODULE_ADC || mode == RT_MODULE_ADC_DAC) {

		res |= rt5670_update_bits(RT5670_STO1_ADC_DIG_VOL,
									RT5670_ADC_L_VOL_MASK | RT5670_ADC_R_VOL_MASK,
									volume<<RT5670_ADC_L_VOL_SFT | volume);
    }
    if (mode == RT_MODULE_DAC || mode == RT_MODULE_ADC_DAC) {
        /*  ... */
    }
    return res;
}

/**
 * @brief Power Management
 *
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int rt5670Start()
{
	int res = 0;
/*
	res |= rt5670WriteReg(RT5670_PWR_ANLG1, 0xf8fd);
	wait_1us(10000);
	res |= rt5670WriteReg(RT5670_PWR_DIG1, 0x980E);
	//res |= rt5670WriteReg(RT5670_PWR_DIG2, 0xD001);
	res |= rt5670WriteReg(RT5670_PWR_ANLG2, 0xEC74);
*/
	return res;
}
/**
 * @brief Power Management
 *
 * @param mod:      if RT_POWER_CHIP, the whole chip including ADC and DAC is enabled
 * @param enable:   false to disable true to enable
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int rt5670Stop()
{
	int res = 0;
/*
	res |= rt5670WriteReg(RT5670_PWR_DIG1, 0x0000);
	//res |= rt5670WriteReg(RT5670_PWR_DIG2, 0x0001);
	res |= rt5670WriteReg(RT5670_PWR_ANLG1, 0x0005);
	res |= rt5670WriteReg(RT5670_PWR_ANLG2, 0x0000);
*/
	return res;
}

union 
{
	struct
	{	
		u16 Reserved:1;		
		u16 Device_id:2;//		
    u16 Reserved2:13;		
	}databit;
	u16 data16;
}MX_REG_00H;


union 
{
	struct
	{	
		//Right Headphone Channel Volume Control (HPOVOLR)
    //00h: +12dB
    //08: 0dB
    //27h: -46.5dB, with 1.5dB/step
		u16 Vol_hpor:6;		
		u16 Reserved:1;//		
		//Mute Control Right Headphone Output Port (HPOR)
		//0: Un-Mute,1b: Mute
    u16 mu_hpo_r:1;		
		//Left Headphone Channel Volume Control (HPOVOLL);
		//00h: +12dB 08: 0dB,... 27: -46.5dB, with 1.5dB/step
    u16 vol_hpol:6;	
		u16 Reserved2:1;//		
		//Mute Control for Left Headphone Output Port (HPOL)
		//0: Un-Mute,1b: Mute
		u16 mu_hpo_l:1;
	}databit;
	u16 data16;
}MX_REG_02H;

union 
{
	struct
	{
		
		//Right Headphone Channel Volume Control (HPOVOLR)
    //00h: +12dB
    //08: 0dB
    //27h: -46.5dB, with 1.5dB/step
		u16 Vol_outr:6;		
		u16 Reserved:1;//		
		//Mute Control Right Headphone Output Port (HPOR)
		//0: Un-Mute,1b: Mute
    u16 Mu_lout_r:1;		
		//Left Headphone Channel Volume Control (HPOVOLL);
		//00h: +12dB 08: 0dB,... 27: -46.5dB, with 1.5dB/step
    u16 Vol_outl:6;	
		u16 En_dfo1:1;//		
		//Mute Control for Left Headphone Output Port (HPOL)
		//0: Un-Mute,1b: Mute
		u16 Mu_lout_l:1;
	}databit;
	u16 data16;
}MX_REG_03H;


union 
{
	struct
	{	
		u16 Reserved2:2;//1h Reserved	
		//IN1 Port Enable Control
		//0b: Disable
		//1b: Enable		
    u16 En_bst1:1;	
		u16 Reserved:9;//0 Reserved		
		//IN1 Boost Control (BST1)
		//0000b: Bypass
		//0001b: +20dB
		//0010b: +24dB
		//0011b: +30dB
		//0100b: +35dB
		//0101b: +40dB
		//0110b: +44dB
		//0111b: +50dB
		//1000b: +52dB
		//Others : Reserved
		u16 Sel_bst1:4;
	}databit;
	u16 data16;
} MX_REG_0AH;

union 
{
	struct
	{
    //27h Reserved		
		u16 Reserved3:7;//1h Reserved	
		//IN1 Port Mode Control
		//0b: Auto mode
		//1b: Manual mode		
    u16 Reg_mode:1;	
		//0h Reserved
		u16 Reserved2:3;//0 Reserved		
		//Capless Power Gating with IN1 Control
		//0b: Register control
		//1b: Auto mode
		u16 Capless_gat_en:1;		
		//Manual Trigger For IN1 Port
		//0b: Low trigger
		//1b: High trigge
		u16 Manual_tri_in1:1;		
		u16 Reserved:3;//0 Reserved		
	}databit;
	u16 data16;
} MX_REG_0BH;

union 
{
	struct
	{
    //27h Reserved		
		u16 In1_result:3;//1h Reserved	
		//0h Reserved
		u16 Reserved:13;//0 Reserved		
	}databit;
	u16 data16;
} MX_REG_0CH;


union 
{
	struct
	{
    //27h Reserved		
		u16 Reserved3:6;//8h Reserved	
		//Enable IN2 Differential Input
		//0b: Disable (Single-end)
		//1b: Enable (Differential	
    u16 En_in2_df:1;		
		//0h Reserved
		u16 Reserved2:1;//0 Reserved			
		//IN2 Boost Control (BST2)
		//0000b: Bypass
		//0001b: +20dB
		//0010b: +24dB
		//0011b: +30dB
		//0100b: +35dB
		//0101b: +40dB
		//0110b: +44dB
		//0111b: +50dB
		//1000b: +52dB
		//Others : Reserved
		u16 Sel_bst2:4;			
		u16 Reserved:4;//0 Reserved		
	}databit;
	u16 data16;
} MX_REG_0DH;

union 
{
	struct
	{
    //27h Reserved		
		u16 Reserved:12;//0h Reserved	
		//IN3 Boost Control (BST2)
		//0000b: Bypass
		//0001b: +20dB
		//0010b: +24dB
		//0011b: +30dB
		//0100b: +35dB
		//0101b: +40dB
		//0110b: +44dB
		//0111b: +50dB
		//1000b: +52dB
		//Others : Reserved
    u16 Sel_bst3:4;			
	}databit;
	u16 data16;
} MX_REG_0EH;


union 
{
	struct
	{	
		//INR Channel Volume Control ?
		//00h: +12dB
		//08h: 0dB
		//1Fh: -34.5dB, with 1.5dB/step
		u16 Vol_inr:5;//		
		u16 Reserved:3;	//0 Reserved	
		//INL Channel Volume Control ?
		//00h: +12dB
		//08h: 0dB
		//1Fh: -34.5dB, with 1.5dB/step
		u16 Vol_inl:5;					
		u16 Reserved2:3;//0h Reserved		
			
	}databit;
	u16 data16;
} MX_REG_0FH;

union 
{
	struct
	{	
		//DAC1 Right Channel Digital Volume?
		//00h: -65.625dB
		//AFh: 0dB, with 0.375dB/Step		
		u16 vol_dac1_r:8;//0h Reserved		
		//DAC1 Left Channel Digital Volume?
		//00h: -65.625dB
		//AFh: 0dB, with 0.375dB/Step
		u16 vol_dac1_l:8;				
	}databit;
	u16 data16;
} MX_REG_19H;

union 
{
	struct
	{	
		//DAC2 Right Channel Digital Volume?
		//00h: -65.625dB
		//AFh: 0dB, with 0.375dB/Step		
		u16 vol_dac2_r:8;//0h Reserved		
		//DAC2 Left Channel Digital Volume?
		//00h: -65.625dB
		//AFh: 0dB, with 0.375dB/Step
		u16 vol_dac2_l:8;				
	}databit;
	u16 data16;
} MX_REG_1AH;

union 
{
	struct
	{	
		//Select DACR2 Data Source
		//000b: IF1_DAC2_R
		//001b: IF2_DAC_R
		//010b: Reserved
		//011b: TxDC_DAC_R
		//100b: TxDP_DAC_L
		u16 Sel_dacr2:3;//1h Reserved		
		u16 Reserved:1;//0h Reserved		
		//Select DACL2 Data Source
		//000b: IF1_DAC2_L
		//001b: IF2_DAC_L
		//010b: Reserved
		//011b: TxDC_DAC_L
		//100b: Reserved
		//101b: VAD_ADC or TxDP_ADC_R		
		u16 Sel_dacl2:3;//1h Reserved		
		u16 Reserved2:5;//0h Reserved
		//Mute Control for Right DAC2 Volume
		//0b: Un-Mute
		//1b: Mute		
		u16 Mu_dac2_r:1;//1h Reserved				
		//DAC2 Left Channel Digital Volume?
		//00h: -65.625dB
		//AFh: 0dB, with 0.375dB/Step	
		//Mute Control for Right DAC2 Volume
		//0b: Un-Mute
		//1b: Mute		
		u16 Mu_dac2_l:1;//1h Reserved				
		u16 Reserved3:2;//0h Reserved			
	}databit;
	u16 data16;
} MX_REG_1BH;


union 
{
	struct
	{	
		//Stereo1 ADC Right Channel Volume Control
		//00h: -17.625dB
		//2Fh: 0dB
		//7Fh: +30dB, with 0.375dB/Step			
		u16 Ad_gain_r:7;//2Fh Reserved	
		//Mute Control for Stereo1 ADC Right Volume Channel
		//0b: Un-Mute
		//1b: Mute	
		u16 Mu_adc_vol_r:1;//1h Reserved		
		//Stereo1 ADC Left Channel Volume Control
		//00h: -17.625dB
		//2Fh: 0dB
		//7Fh: +30dB, with 0.375dB/Step	
		u16 Ad_gain_l:7;//2Fh Reserved		
		//Mute Control for Stereo1 ADC Left Volume Channel
		//0b: Un-Mute
		//1b: Mute		
		u16 Mu_adc_vol_l:1;//0h Reserved				
	}databit;
	u16 data16;
} MX_REG_1CH;

union 
{
	struct
	{	
		//Stereo1 ADC Right Channel Volume Control
		//00h: -17.625dB
		//2Fh: 0dB
		//7Fh: +30dB, with 0.375dB/Step			
		u16 Mono_ad_gain_r:7;//2Fh Reserved	
		//Mute Control for Stereo1 ADC Right Volume Channel
		//0b: Un-Mute
		//1b: Mute	
		u16 reserved:1;//0h Reserved		
		//Stereo1 ADC Left Channel Volume Control
		//00h: -17.625dB
		//2Fh: 0dB
		//7Fh: +30dB, with 0.375dB/Step	
		u16 Mono_ad_gain_l:7;//2Fh Reserved		
		//Mute Control for Stereo1 ADC Left Volume Channel
		//0b: Un-Mute
		//1b: Mute		
		u16 reserved2:1;//0h Reserved				
	}databit;
	u16 data16;
} MX_REG_1DH;	

union 
{
	struct
	{			
		u16 reserved:4;//0h Reserved	
		//Stereo2 ADC Compensation Gain
		//00b: 0dB
		//01b: 1dB
		//10b: 2dB
		//11b: 3dB
		u16 Stereo2_ad_comp_gain:2;//0h Reserved		
		//Stereo2 ADC Right Channel Digital Boost Gain
		//00b: 0dB
		//01b: 12dB
		//10b: 24dB
		//11b: 36dB
		u16 Stereo2_ad_boost_gain_r:2;//0h Reserved	
		//Stereo2 ADC Left Channel Digital Boost Gain
		//00b: 0dB
		//01b: 12dB
		//10b: 24dB
		//11b: 36d		
		u16 Stereo2_ad_boost_gain_l:2;//0h Reserved		
		//Stereo1 ADC Compensation Gain
		//00b: 0dB
		//01b: 1dB
		//10b: 2dB
		//11b: 3dB
		u16 Stereo1_ad_comp_gain:2;//0h Reserved	
		//Stereo1 ADC Right Channel Digital Boost Gain
		//00b: 0dB
		//01b: 12dB
		//10b: 24dB
		//11b: 36dB		
		u16 Stereo1_ad_boost_gain_r:2;//0h Reserved		
		//Stereo1 ADC Left Channel Digital Boost Gain
		//00b: 0dB
		//01b: 12dB
		//10b: 24dB
		//11b: 36dB		
		u16 Stereo1_ad_boost_gain_l:2;//0h Reserved				

	}databit;
	u16 data16;
} MX_REG_1EH;


union 
{
	struct
	{
		//Stereo2 ADC Right Channel Volume Control
		//00h: -17.625dB
		//2Fh: 0dB
		//7Fh: +30dB, with 0.375dB/Step		
		u16 Ad2_gain_r:7;//2Fh Reserved	
		
		//Mute Control for Stereo2 ADC Right Volume Channel
		//0b: Un-Mute
		//1b: Mute
		u16 Mu_adc2_vol_r:1;//0h Reserved		
		
		//Stereo2 ADC Left Channel Volume Control
		//00h: -17.625dB
		//2Fh: 0dB
		//7Fh: +30dB, with 0.375dB/Step		
		u16 Ad2_gain_l:7;//2Fh Reserved	
		
		//Mute Control for Stereo2 ADC Left Volume Channel
		//0b: Un-Mute
		//1b: Mute	
		u16 Mu_adc2_vol_l:1;//0h Reserved		
		
	}databit;
	u16 data16;
} MX_REG_1FH;


union 
{
	struct
	{
		//Stereo2 ADC Right Channel Volume Control
		//00h: -17.625dB
		//2Fh: 0dB
		//7Fh: +30dB, with 0.375dB/Step		
		u16 Reserved:10;//0h Reserved	
		
		//Mono ADC Compensation Gain
		//00b: 0dB
		//01b: 1dB
		//10b: 2dB
		//11b: 3dB
		u16 mono_ad_comp_gain:2;//0h Reserved		
		
		//Mono ADC Right Channel Digital Boost Gain
		//00b: 0dB
		//01b: 12dB
		//10b: 24dB
		//11b: 36dB	
		u16 mono_ad_boost_gain_r:2;//2Fh Reserved	
		
		//Mono ADC Left Channel Digital Boost Gain
		//00b: 0dB
		//01b: 12dB
		//10b: 24dB
		//11b: 36dB
		u16 mono_ad_boost_gain_l:2;//0h Reserved		
		
	}databit;
	u16 data16;
} MX_REG_20H;

union 
{
	struct
	{
		u16 reserved:5;//0h Reserved	
		u16 mu_stereo2_adcr2:1;//0h Reserved		
		u16 mu_stereo2_adcr1:1;//2Fh Reserved		
		u16 reserved2:1;//0h Reserved		
		u16 Sel_stereo2_dmic:2;//0h Reserved	
		u16 Sel_stereo2_adc:1;//0h Reserved		
		u16 sel_stereo2_adc2:1;//2Fh Reserved		
		u16 sel_stereo2_adc1:1;//0h Reserved				
		u16 mu_stereo2_adcl2:1;//0h Reserved		
		u16 mu_stereo2_adcl1:1;//0h Reserved	
		u16 Sel_stereo2_lr_mix:1;//0h Reserved			
			
	}databit;
	u16 data16;
} MX_REG_26H;

union 
{
	struct
	{
		u16 Sel_dmic3_data:1;//0h Reserved			
		u16 reserved:4;//2Fh Reserved		
		u16 mu_stereo1_adcr2:1;//0h Reserved		
		u16 mu_stereo1_adcr1:1;//0h Reserved			
		u16 reserved2:1;//0h Reserved		
		u16 Sel_stereo1_dmic:2;//2Fh Reserved		
		u16 Sel_stereo1_adc:1;//0h Reserved			
		u16 sel_stereo1_adc2:1;//0h Reserved		
		u16 sel_stereo1_adc1:1;//0h Reserved	
		u16 mu_stereo1_adcl2:1;//0h Reserved			
		u16 mu_stereo1_adcl1:1;//0h Reserved	
		u16 reserved3:1;//0h Reserved			
	}databit;
	u16 data16;
} MX_REG_27H;

union 
{
	struct
	{
		u16 Sel_mono_dmic_r:2;// 			
		u16 sel_mono_adc_r:1;// 		
		u16 sel_mono_adcr2:1;// 			
		u16 sel_mono_adcr1:1;// 	
		u16 mu_mono_adcr2:1;// 		
		u16 mu_mono_adcr1:1;// 		
		u16 Reserved:1;// 					
		u16 Sel_mono_dmic_l:2;// 		
		u16 sel_mono_adc_l:1;// 	
		u16 sel_mono_adcl2:1;// 			
		u16 sel_mono_adcl1:1;// 	
		u16 mu_mono_adcl2:1;// 			
		u16 mu_mono_adcl1:1;// 	
		u16 reserved:1;// 		
	}databit;
	u16 data16;
} MX_REG_28H;



union 
{
	struct
	{
		u16 reserved:6;//0h Reserved			
		//Mute Source1 to Stereo2 ADC Right Channel
		//0b:UnMute
		//1b:Mute	
		u16 Mu_dac1_r:1;//2Fh Reserved	
		
		u16 Mu_stereo1_adc_mixer_r:1;//0h Reserved	

		//Select Stereo2 DMIC Source
		//00b: DMIC1
		//01b: DMIC2
		//10b: DMIC3
		//11b: Reserved		
		u16 Sel_dacl1:2;//0h Reserved	
		
		//Select Stereo2 ADC Filter Source
		//0b: ADC1 => Left channel
		//ADC2 => Right channel
		//1b: ADC3 => Left channel
		//ADC3 => Right channel
		u16 Sel_dacr1:2;//0h Reserved		
		
		//Select Stereo2 ADC L/R Channel Source 2
		//0b: DAC_MIXL / DAC_MIXR
		//1b: DMIC1/DMIC2/DMIC3
		u16 reserved2:2;//2Fh Reserved	
		//Select Stereo2 ADC L/R Channel Source 1
		//0b: DAC_MIXL / DAC_MIXR
		//1b: ADC1		
		u16 Mu_dac1_l:1;//0h Reserved			
		//Mute Source2 to Stereo2 ADC Left Channel
		//0b:UnMute
		//1b:Mute		
		u16 Mu_stereo1_adc_mixer_l:1;//0h Reserved	
		


	}databit;
	u16 data16;
} MX_REG_29H;




union 
{
	struct
	{
		u16 gain_dacl1_to_stereo_r:1;//0h Reserved			
		//Mute Source1 to Stereo2 ADC Right Channel
		//0b:UnMute
		//1b:Mute	
		u16 mu_stereo_dacl1_mixr:1;//2Fh Reserved	
		
		u16 Mu_snc_to_dac_r:1;//0h Reserved	

		//Select Stereo2 DMIC Source
		//00b: DMIC1
		//01b: DMIC2
		//10b: DMIC3
		//11b: Reserved		
		u16 gain_dacr2_to_stereo_r:1;//0h Reserved	
		
		//Select Stereo2 ADC Filter Source
		//0b: ADC1 => Left channel
		//ADC2 => Right channel
		//1b: ADC3 => Left channel
		//ADC3 => Right channel
		u16 mu_stereo_dacr2_mixr:1;//0h Reserved		
		
		//Select Stereo2 ADC L/R Channel Source 2
		//0b: DAC_MIXL / DAC_MIXR
		//1b: DMIC1/DMIC2/DMIC3
		u16 gain_dacr1_to_stereo_r:1;//2Fh Reserved	
		//Select Stereo2 ADC L/R Channel Source 1
		//0b: DAC_MIXL / DAC_MIXR
		//1b: ADC1		
		u16 mu_stereo_dacr1_mixr:1;//0h Reserved			
		//Mute Source2 to Stereo2 ADC Left Channel
		//0b:UnMute
		//1b:Mute		
		u16 reserved:1;//0h Reserved	
		//Mute Source1 to Stereo2 ADC Left Channel
		//0b:UnMute
		//1b:Mute		
		u16 gain_dacr1_to_stereo_l:1;//0h Reserved
		//Mixing Control for Stereo2 ADC Left channel
		//0b: L
		//1b: L+R		
		u16 mu_stereo_dacr1_mixl:1;//0h Reserved			

			
		u16 Mu_snc_to_dac_l:1;//0h Reserved	
		u16 gain_dacl2_to_stereo_l:1;//0h Reserved		

		u16 mu_stereo_dacl2_mixl:1;//0h Reserved	
		u16 gain_dacl1_to_stereo_l:1;//0h Reserved			

		u16 mu_stereo_dacl1_mixl:1;//0h Reserved	
		u16 reserved2:1;//0h Reserved	

	}databit;
	u16 data16;
} MX_REG_2AH;

union 
{
	struct
	{
		u16 reserved:1;//0h Reserved			
		//Mute Source1 to Stereo2 ADC Right Channel
		//0b:UnMute
		//1b:Mute	
		u16 gain_mono_r_dacl2:1;//2Fh Reserved	
		
		u16 mu_mono_dacl2_mixr:1;//0h Reserved	

		//Select Stereo2 DMIC Source
		//00b: DMIC1
		//01b: DMIC2
		//10b: DMIC3
		//11b: Reserved		
		u16 gain_mono_r_dacr2:1;//0h Reserved	
		
		//Select Stereo2 ADC Filter Source
		//0b: ADC1 => Left channel
		//ADC2 => Right channel
		//1b: ADC3 => Left channel
		//ADC3 => Right channel
		u16 mu_mono_dacr2_mixr:1;//0h Reserved		
		
		//Select Stereo2 ADC L/R Channel Source 2
		//0b: DAC_MIXL / DAC_MIXR
		//1b: DMIC1/DMIC2/DMIC3
		u16 gain_mono_r_dacr1:1;//2Fh Reserved	
		//Select Stereo2 ADC L/R Channel Source 1
		//0b: DAC_MIXL / DAC_MIXR
		//1b: ADC1		
		u16 mu_mono_dacr1_mixr:1;//0h Reserved			
		//Mute Source2 to Stereo2 ADC Left Channel
		//0b:UnMute
		//1b:Mute		
		u16 reserved2:2;//0h Reserved	
		//Mute Source1 to Stereo2 ADC Left Channel
		//0b:UnMute
		//1b:Mute		
		u16 gain_mono_l_dacr2:1;//0h Reserved
		//Mixing Control for Stereo2 ADC Left channel
		//0b: L
		//1b: L+R		
		u16 mu_mono_dacr2_mixl:1;//0h Reserved			
		
		u16 gain_mono_l_dacl2:1;//0h Reserved	
		u16 mu_mono_dacl2_mixl:1;//0h Reserved		

		u16 gain_mono_l_dacl1:1;//0h Reserved	
		u16 mu_mono_dacl1_mixl:1;//0h Reserved			

		u16 reserved3:1;//0h Reserved	

	}databit;
	u16 data16;
} MX_REG_2BH;




union 
{
	struct
	{
		u16 reserved:4;//0h Reserved			
		//Mute Source1 to Stereo2 ADC Right Channel
		//0b:UnMute
		//1b:Mute	
		u16 gain_dacl2_to_dacmixr:1;//2Fh Reserved	
		
		u16 Mu_dacl2_to_dacmixr:1;//0h Reserved	

		//Select Stereo2 DMIC Source
		//00b: DMIC1
		//01b: DMIC2
		//10b: DMIC3
		//11b: Reserved		
		u16 gain_dacr2_to_dacmixl:1;//0h Reserved	
		
		//Select Stereo2 ADC Filter Source
		//0b: ADC1 => Left channel
		//ADC2 => Right channel
		//1b: ADC3 => Left channel
		//ADC3 => Right channel
		u16 Mu_dacr2_to_dacmixl:1;//0h Reserved		
		
		//Select Stereo2 ADC L/R Channel Source 2
		//0b: DAC_MIXL / DAC_MIXR
		//1b: DMIC1/DMIC2/DMIC3
		u16 gain_dacr2_to_dacmixr:1;//2Fh Reserved	
		//Select Stereo2 ADC L/R Channel Source 1
		//0b: DAC_MIXL / DAC_MIXR
		//1b: ADC1		
		u16 Mu_dacr2_to_dacmixr:1;//0h Reserved			
		//Mute Source2 to Stereo2 ADC Left Channel
		//0b:UnMute
		//1b:Mute		
		u16 gain_stereomixr_to_dacmixr:1;//0h Reserved	
		//Mute Source1 to Stereo2 ADC Left Channel
		//0b:UnMute
		//1b:Mute		
		u16 Mu_stereomixr_to_dacmixr:1;//0h Reserved
		//Mixing Control for Stereo2 ADC Left channel
		//0b: L
		//1b: L+R		
		u16 gain_dacl2_to_dacmixl:1;//0h Reserved					
		u16 Mu_dacl2_to_dacmixl:1;//0h Reserved	
		
		u16 gain_stereomixl_to_dacmixl:1;//0h Reserved		
		u16 Mu_stereomixl_to_dacmixl:1;//0h Reserved	

	}databit;
	u16 data16;
} MX_REG_2CH;

union 
{
	struct
	{
		u16 Sel_dsp_dl_bypass:1;//0h Reserved			
		//Mute Source1 to Stereo2 ADC Right Channel
		//0b:UnMute
		//1b:Mute	
		u16 Sel_dsp_ul_bypass:1;//2Fh Reserved	
		
		u16 Sel_tdm_txdp_slot:2;//0h Reserved	

		//Select Stereo2 DMIC Source
		//00b: DMIC1
		//01b: DMIC2
		//10b: DMIC3
		//11b: Reserved		
		u16 Sel_src_to_txdp:2;//0h Reserved	
		
		//Select Stereo2 ADC Filter Source
		//0b: ADC1 => Left channel
		//ADC2 => Right channel
		//1b: ADC3 => Left channel
		//ADC3 => Right channel
		u16 Sel_txdc_data:2;//0h Reserved		
		
		//Select Stereo2 ADC L/R Channel Source 2
		//0b: DAC_MIXL / DAC_MIXR
		//1b: DMIC1/DMIC2/DMIC3
		u16 Sel_txdp_data:2;//2Fh Reserved	
		//Select Stereo2 ADC L/R Channel Source 1
		//0b: DAC_MIXL / DAC_MIXR
		//1b: ADC1		
		u16 Reserved:1;//0h Reserved			
		//Mute Source2 to Stereo2 ADC Left Channel
		//0b:UnMute
		//1b:Mute		
		u16 Sel_src_to_rxdp:2;//0h Reserved	
		//Mute Source1 to Stereo2 ADC Left Channel
		//0b:UnMute
		//1b:Mute		
		u16 Sel_rxdp_in:3;//0h Reserved
	}databit;
	u16 data16;
} MX_REG_2DH;


union 
{
	struct
	{
		u16 Vol_txdp_r:7;//0h Reserved			
		//Mute Source1 to Stereo2 ADC Right Channel
		//0b:UnMute
		//1b:Mute	
		u16 Reserved:1;//2Fh Reserved	
		
		u16 Vol_txdp_l:7;//0h Reserved	

		//Select Stereo2 DMIC Source
		//00b: DMIC1
		//01b: DMIC2
		//10b: DMIC3
		//11b: Reserved		
		u16 Reserved2:1;//0h Reserved	
	}databit;
	u16 data16;
} MX_REG_2EH;

union 
{
	struct
	{
		u16 Sel_if3_adc_data_in:3;//0h Reserved			
		u16 Reserved:1;//2Fh Reserved		
		u16 sel_if3_adc_data:2;//0h Reserved		
		u16 sel_if3_dac_data:2;//0h Reserved	
		u16 sel_if2_adc_data:2;//0h Reserved	
		u16 sel_if2_dac_data:2;//0h Reserved			
		u16 Sel_if2_adc_data_in:3;//0h Reserved	
		u16 Sel_if1_adc2_data_in:1;//0h Reserved		
	}databit;
	u16 data16;
} MX_REG_2FH;

union 
{
	struct
	{
		u16 Reserved:4;//0h Reserved			
		u16 Gain_pdm_in:1;//2Fh Reserved		
		u16 Sel_pdm_pattern_ctrl:1;//0h Reserved		
		u16 reserved:2;//0h Reserved	
		
		u16 mu_pdm2_r:1;//0h Reserved	
		u16 sel_pdm2_r:1;//0h Reserved			
		u16 mu_pdm2_l:1;//0h Reserved	
		u16 sel_pdm2_l:1;//0h Reserved
		u16 reserved2:4;//0h Reserved	
		
	}databit;
	u16 data16;
} MX_REG_31H;


union 
{
	struct
	{
		u16 Reserved:3;//0h Reserved			
		u16 Pdm2_cmd_exe:1;//2Fh Reserved		
		u16 Reserved2:12;//0h Reserved	
		
	}databit;
	u16 data16;
} MX_REG_32H;

union 
{
	struct
	{
		u16 Pdm2_cmd_pattern:8;//0h Reserved			
		u16 Reserved:8;//2Fh Reserved			
	}databit;
	u16 data16;
} MX_REG_35H;

union 
{
	struct
	{
		u16 reserved:1;//			
		u16 gain_bst2_recmixl:3;//	

		u16 gain_bst3_recmixl:3;//
		u16 reserved2:3;//
		u16 gain_inl_recmixl:3;//						
		u16 reserved3:3;//	
		
	}databit;
	u16 data16;
} MX_REG_3BH;

union 
{
	struct
	{
		u16 reserved:1;//			
		u16 Mu_bst1_recmixl:1;//	
		u16 Mu_bst2_recmixl:1;//
		u16 Mu_bst3_recmixl:1;//
		u16 reserved2:1;//						
		u16 Mu_inl_recmixl:1;//	
		u16 reserved3:7;//						
		u16 gain_bst1_recmixl:3;//		
	}databit;
	u16 data16;
} MX_REG_3CH;


union 
{
	struct
	{
		u16 reserved:1;//			
		u16 gain_bst2_recmixr:3;//	
		u16 gain_bst3_recmixr:3;//
		u16 reserved2:3;//
		u16 gain_inr_recmixr:3;//						
		u16 reserved3:3;//			
	}databit;
	u16 data16;
} MX_REG_3DH;

union 
{
	struct
	{
		u16 reserved:1;//			
		u16 Mu_bst1_recmixr:1;//	
		u16 Mu_bst2_recmixr:1;//
		u16 Mu_bst3_recmixr:1;//
		u16 reserved2:1;//			
		u16 Mu_inr_recmixr:1;//		
		
		u16 reserved3:7;//	
		u16 gain_bst1_recmixr:3;//	
		
	}databit;
	u16 data16;
} MX_REG_3EH;


union 
{
	struct
	{
		u16 reserved:1;//			
		u16 gain_bst2_recmixr:3;//	
		u16 gain_bst3_recmixr:3;//
		u16 reserved2:9;//		
	}databit;
	u16 data16;
} MX_REG_3FH;

union 
{
	struct
	{
		u16 reserved:1;//			
		u16 Mu_bst1_recmixm:1;//	
		u16 Mu_bst2_recmixm:1;//
		u16 Mu_bst3_recmixm:1;//	
		u16 reserved2:9;//
		u16 gain_bst1_recmixm:3;//		
	}databit;
	u16 data16;
} MX_REG_40H;

union 
{
	struct
	{
		u16 Mu_dacl1_hpomixl:1;//			
		u16 Mu_inl1_hpomixl:1;//	
		u16 Mu_dacr1_hpomixr:1;//
		u16 Mu_inr1_hpmixr:1;//
		u16 reserved:8;//
		
		u16 En_bst_hp:1;//
		u16 Mu_hpvol_hpo:1;//	

		u16 Mu_dac1_hpo:1;//
		u16 reserved2:1;//	
		
	}databit;
	u16 data16;
} MX_REG_45H;


union 
{
	struct
	{
		u16 Mu_dacl1_outmixl1:1;//			
		u16 Mu_dacl2_outmixl1:1;//	
		u16 Reserved:2;//
		u16 Mu_inl_outmixl1:1;//		
		u16 Mu_bst1_outmixl1:1;//		
		u16 Mu_bst2_outmixl1:1;//	
		u16 Reserved2:9;//	
	}databit;
	u16 data16;
} MX_REG_4FH;


union 
{
	struct
	{
		u16 Mu_dacr1_outmixr1:1;//			
		u16 Mu_dacr2_outmixr1:1;//	
		u16 Reserved:2;//
		u16 Mu_inr_outmixr1:1;//		
		u16 Reserved2:1;//		
		u16 Mu_bst3_outmixr1:1;//	
		u16 Reserved3:9;//	
	}databit;
	u16 data16;
} MX_REG_52H;

union 
{
	struct
	{
		u16 reserved:11;//			
		u16 bst_lout1:1;//	
		u16 mu_outmixr1_lout1:1;//
		u16 mu_outmixl1_lout1:1;//		
		u16 mu_dacr1_lout1:1;//		
		u16 mu_dacl1_lout1:1;//	

	}databit;
	u16 data16;
} MX_REG_53H;


union 
{
	struct
	{
		u16 reserved:1;//			
		u16 Pow_adc_r:1;//	
		u16 Pow_adc_l:1;//
		u16 Pow_adc_3:1;//		
		u16 reserved2:2;//		
		u16 Pow_dac_r_2:1;//	
		u16 Pow_dac_l_2:1;//		
		u16 reserved3:3;//			
		u16 Pow_dac_r_1:1;//			
		u16 Pow_dac_l_1:1;//
		u16 En_i2s3:1;//		
		u16 En_i2s2:1;//		
		u16 En_i2s1:1;//		
	}databit;
	u16 data16;
} MX_REG_61H;	

union 
{
	struct
	{
		u16 reserved:1;//			
		u16 En_serial_if:1;//	
		u16 reserved2:4;//
		u16 Pow_pdm2:1;//		
		u16 Pow_pdm1:1;//	
		
		u16 pow_adc_stereo2_filter:1;//	
		u16 pow_dac_monor_filter:1;//	
		
		u16 pow_dac_monol_filter:1;//			
		u16 pow_dac_stereo1_filter:1;//	
			
		u16 Pow_i2s_dsp:1;//
		u16 Pow_adc_monor_filter:1;//		
		u16 Pow_adc_monol_filter:1;//		
		u16 Pow_adc_stereo_filter:1;//	
		
	}databit;
	u16 data16;
} MX_REG_62H;


union 
{
	struct
	{
		u16 Dvo_ldo:3;//			
		u16 En_fastb2:1;//	
		u16 Pow_vref2:1;//
		u16 En_amp_hp:1;//		
		u16 En_r_hp:1;//			
		u16 En_l_hp:1;//	
		u16 reserved:3;//		
		u16 Pow_bg_bias:1;//			
		u16 Pow_lout:1;//				
		u16 Pow_main_bias:1;//
		u16 En_fastb1:1;//		
		u16 Pow_vref1:1;//		
		
	}databit;
	u16 data16;
} MX_REG_63H;


union 
{
	struct
	{
		u16 reserved:1;//			
		u16 Pow_jd2:1;//	
		u16 Pow_jd1:1;//
		u16 reserved2:1;//	
		
		u16 Pow_bst3_2:1;//			
		u16 Pow_bst2_2:1;//
		
		u16 Pow_bst1_2:1;//	
		
		u16 reserved3:2;//
		
		u16 Pow_pll:1;//
		
		u16 Pow_micbias2:1;//
		
		u16 Pow_micbias1_1:1;//		
		u16 reserved4:1;//		
		
		
		u16 Pow_bst3_1:1;//		
		u16 Pow_bst2_1:1;//	
		u16 Pow_bst1_1:1;//		
			
	}databit;
	u16 data16;
} MX_REG_64H;


union 
{
	struct
	{
		u16 reserved:9;//			
		u16 Pow_recmixm:1;//	
		u16 Pow_recmixr:1;//
		u16 Pow_recmixl:1;//	
		
		u16 reserved2:2;//			
		u16 Pow_outmixr:1;//		
		u16 Pow_outmixl:1;//	
					
	}databit;
	u16 data16;
} MX_REG_65H;



union 
{
	struct
	{
		u16 Pow_micbias1_2:1;//			
		u16 Pow_micbias2_2:1;//	
		u16 reserved:3;//
		u16 Pow_mic_in_det:1;//	
		u16 reserved2:2;//
		u16 Pow_inrvol:1;//			
		u16 Pow_inlvol:1;//
		
		u16 Pow_hpovolr:1;//		
		u16 Pow_hpovoll:1;//	
		u16 reserved3:4;//			
	}databit;
	u16 data16;
} MX_REG_66H;


union 
{
	struct
	{		
		u16 Pr_index:8;//	
		u16 reserved:8;//			
	}databit;
	u16 data16;
} MX_REG_6AH;

union 
{
	struct
	{		
		u16 Pr_data:16;//			
	}databit;
	u16 data16;
} MX_REG_6CH;

union 
{
	struct
	{	
		u16 sel_i2s1_format:2;//	
		u16 sel_i2s1_len:2;//	
		u16 reserved:3;//	
		u16 Inv_i2s1_bclk:1;//	
		u16 en_i2s1_in_comp:2;//	
		u16 en_i2s1_out_comp:2;//	
		u16 reserved2:3;//	
		u16 Sel_i2s1_ms:1;//	
		
	}databit;
	u16 data16;
} MX_REG_70H;

union 
{
	struct
	{	
		u16 sel_i2s2_format:2;//	
		u16 sel_i2s2_len:2;//	
		u16 reserved:3;//	
		u16 inv_i2s2_bclk:1;//	
		u16 en_i2s2_in_comp:2;//	
		u16 en_i2s2_out_comp:2;//	
		u16 reserved2:3;//	
		u16 Sel_i2s2_ms:1;//	
		
	}databit;
	u16 data16;
} MX_REG_71H;

union 
{
	struct
	{	
		u16 sel_i2s3_format:2;//	
		u16 sel_i2s3_len:2;//	
		u16 reserved:3;//	
		u16 inv_i2s3_bclk:1;//
		
		u16 en_i2s3_in_comp:2;//	
		u16 en_i2s3_out_comp:2;//	
		u16 reserved2:3;//	
		u16 Sel_i2s3_ms:1;//	
	
	}databit;
	u16 data16;
} MX_REG_72H;

union 
{
	struct
	{	
		u16 sel_adc_osr:2;//	
		u16 sel_dac_osr:2;//	
		u16 sel_i2s_pre_div3:3;//	
		u16 sel_i2s_bclk_ms3:1;//
		
		u16 sel_i2s_pre_div2:3;//	
		u16 sel_i2s_bclk_ms2:1;//	
		
		u16 sel_i2s_pre_div1:3;//	
		u16 reserved:1;//	
		
	}databit;
	u16 data16;
} MX_REG_73H;


union 
{
	struct
	{	
		u16 Reserved:9;//	
		u16 Mono_adhpf_en:1;//	
		u16 Adhpf_en:1;//	
		u16 Dehpf_en:1;//	
		u16 reserved:4;//	
			
	}databit;
	u16 data16;
} MX_REG_74H;



union 
{
	struct
	{
		u16 dmic1_data_pin_share:2;//			
		u16 sel_dmic_r_edge_mono:1;//	
		u16 sel_dmic_l_edge_mono:1;//	
		u16 en_dmic3:1;//		
		u16 sel_dmic_clk:3;//			
		u16 sel_dmic_r_edge_stereo2:1;//
			
		u16 sel_dmic_l_edge_stereo2:1;//		
		u16 reserved:2;//	
			
		u16 sel_dmic_r_edge_stereo1:1;//
		u16 sel_dmic_l_edge_stereo1:1;//
		u16 en_dmic2:1;//
		u16 en_dmic1:1;//	
		
	}databit;
	u16 data16;
} MX_REG_75H;


union 
{
	struct
	{	
		
		u16 sel_dmic1_lpf_r_edge:1;//
		u16 sel_dmic1_lpf_l_edge:1;//	
		
		u16 sel_dmic2_lpf_r_edge:1;//
		u16 sel_dmic2_lpf_l_edge:1;//	
		
		u16 sel_dmic3_lpf_r_edge:1;//	
		u16 sel_dmic3_lpf_l_edge:1;//		
		
		u16 Dmic3_data_pin_share:2;//
		u16 Reserved:8;//						
	}databit;
	u16 data16;
} MX_REG_76H;


union 
{
	struct
	{	
		
		u16 sel_i2s_rx_ch8:2;//		
		u16 sel_i2s_rx_ch6:2;//	
		
		u16 sel_i2s_rx_ch4:2;//		
		u16 sel_i2s_rx_ch2:2;//	
		
		u16 reserved2:1;//				
		u16 rx_adc_data_sel:1;//
		
		u16 Channel_length:2;//		
		u16 Tdmslot_sel:2;//	
		
		u16 mode_sel:1;//			
		u16 reserved:1;//			
		
	}databit;
	u16 data16;
} MX_REG_77H;

union 
{
	struct
	{
		u16 mute_tdm8_outr:1;//	
		u16 mute_tdm8_outl:1;//	
		
		u16 mute_tdm6_outr:1;//
		u16 mute_tdm6_outl:1;//		
		
		u16 mute_tdm4_outr:1;//	
		u16 mute_tdm4_outl:1;//	
		
		u16 mute_tdm2_outr:1;//
		u16 mute_tdm2_outl:1;//	
		
		u16 reserved2:3;//		
		u16 lrck_pulse_sel:1;//		
		
		u16 reserved:3;//	
		u16 sel_i2s_lrck_polarity:1;//				
	}databit;
	u16 data16;
} MX_REG_78H;

union 
{
	struct
	{

		u16 sel_i2s_tx_r_ch4:3;//			
		u16 Reserved:1;//
		
		u16 sel_i2s_tx_l_ch4:3;//			
		u16 Reserved2:1;//		
		
		u16 sel_i2s_tx_r_ch2:3;//			
		u16 Reserved3:1;//
				
		u16 sel_i2s_tx_l_ch2:3;//			
		u16 Reserved4:1;//	
				
	}databit;
	u16 data16;
} MX_REG_79H;

union 
{
	struct
	{		
		u16 sel_dsp_track0:4;//	
		u16 reserved:4;//
		u16 sel_i2s_pre_div5:3;//		
		u16 reserved2:1;//	
		u16 sel_i2s_pre_div4:3;//
		u16 sel_i2s_bclk_ms4:1;//				
	}databit;
	u16 data16;
} MX_REG_7FH;

union 
{
	struct
	{	
		u16 reserved:3;//
		u16 sel_pll_pre_div:1;//		
		u16 reserved2:7;//
		u16 sel_pll_sour:3;//	
		u16 sel_sysclk1:2;//	
	}databit;
	u16 data16;
} MX_REG_80H;


union 
{
	struct
	{	
		u16 Pll_k_code:5;//
		u16 Reserved:2;//		
		u16 Pll_n_code:9;//	
	}databit;
	u16 data16;
} MX_REG_81H;

union 
{
	struct
	{	
		u16 Reserved:11;//
		u16 Pll_m_bypass:1;//		
		u16 Pll_m_code:4;//	
	}databit;
	u16 data16;
} MX_REG_82H;

union 
{
	struct
	{	
		u16 en_adc_asrc_monor:1;//
		u16 en_adc_asrc_monol:1;//		
		u16 en_adc_asrc_stereo2:1;//	
		
		u16 en_adc_asrc_stereo1:1;//
		u16 en_dmic_asrc_monor:1;//		
		u16 en_dmic_asrc_monol:1;//	

		u16 en_dmic_asrc_stereo2:1;//
		
		u16 en_dmic_asrc_stereo1:1;//		
		u16 Sel_mono_dac_r_mode:1;//

		u16 Sel_mono_dac_l_mode:1;//
		u16 Sel_stereo_dac_mode :1;//	
	
		u16 En_i2s1_asrc:1;//
		u16 En_i2s2_asrc:1;//
		u16 En_i2s3_asrc:1;//
		u16 Reserved:2;//
		
	}databit;
	u16 data16;
} MX_REG_83H;


union 
{
	struct
	{	
		u16 sel_ad_filter_stereo1_asrc:4;//
		u16 sel_da_filter_monor_asrc:4;//	
		
		u16 sel_da_filter_monol_asrc:4;//		
		u16 sel_da_filter_stereo_asrc:4;//
			
	}databit;
	u16 data16;
} MX_REG_84H;

union 
{
	struct
	{	
		u16 sel_ad_filter_monor_asrc:4;//
		u16 sel_ad_filter_monol_asrc:4;//	
		
		u16 sel_down_filter_asrc:4;//		
		u16 sel_up_filter_asrc:4;//
			
	}databit;
	u16 data16;
} MX_REG_85H;

union 
{
	struct
	{	
		u16 reserved:4;//
		u16 sel_i2s3_asrc:2;//	
		
		u16 i2s3_asrc_prediv:2;//		
		u16 sel_i2s2_asrc:2;//
	
		u16 i2s2_asrc_prediv:2;//
		u16 sel_i2s1_asrc:2;//	
		
		u16 i2s1_asrc_prediv:2;//		
	}databit;
	u16 data16;
} MX_REG_8AH;

union 
{
	struct
	{	
		u16 reserved:12;//
		u16 sel_ad_filter_stereo2_asrc:4;//	
				
	}databit;
	u16 data16;
} MX_REG_8CH;

union 
{
	struct
	{	
		u16 Pow_capless:1;//
		u16 reserved:1;//	
		u16 En_softgen_hp:1;//
		
		u16 Pow_pump_hp:1;//	
		u16 En_out_hp:1;//
		u16 Softgen_rstp:1;//	
		u16 Softgen_rstn:1;//
		u16 Pdn_hp:1;//	
		u16 En_smt_r_hp:1;//
		u16 En_smt_l_hp:1;//	
		u16 reserved2:5;//
		u16 Smttrig_hp:1;//			

		
	}databit;
	u16 data16;
} MX_REG_8EH;


union 
{
	struct
	{	
		u16 reserved:6;//
		u16 En_depop_mode1:1;//	
		u16 reserved2:6;//
		
		u16 Depop_mode_hp:1;//	
		u16 reserved3:2;//
		
		
	}databit;
	u16 data16;
} MX_REG_8FH;


union 
{
	struct
	{	
		u16 reserved:8;//	
		u16 Sel_pm_hp:2;//	
		u16 reserved3:6;//	
	}databit;
	u16 data16;
} MX_REG_91H;

union 
{
	struct
	{	
		u16 reserved:3;//	
		u16 Sel_irq_debounce:1;//	
		u16 Pow_clk_int:1;//

		u16 Ckn_micbias:1;//	
		u16 Mic2_ovcd_th_sel:2;//	
		u16 Pow_mic2_ovcd:1;//
		u16 Mic1_ovcd_th_sel:2;//	
		u16 Pow_mic1_ovcd:1;//	
		u16 reserved2:2;//
		u16 Sel_micbias2:1;//	
		u16 Sel_micbias1:1;//	
	
	}databit;
	u16 data16;
} MX_REG_93H;

union 
{
	struct
	{	
		u16 Sel_mode_jd1:2;//	
		u16 reserved:14;//		
	}databit;
	u16 data16;
} MX_REG_94H;

union 
{
	struct
	{	
		u16 Reserved:2;//	
		u16 En_sld_reset:1;//		
		u16 En_sld_sys:1;//	
		u16 En_sld_det:1;//	
		u16 En_sld_enc:1;//	
		u16 En_sld_dec:1;//	
		u16 En_sld_clr:1;//	
		u16 En_sld_buf_ow:1;//	
		u16 En_sld_fg2enc:1;//	
		u16 reserved:6;//	
	}databit;
	u16 data16;
} MX_REG_9AH;
 
union 
{
	struct
	{	
		u16 Sel_sld_samll_lv:8;//	
		u16 reserved:8;//		
	}databit;
	u16 data16;
} MX_REG_9BH;

union 
{
	struct
	{	
		u16 Sel_sld_lv_var:8;//	
		u16 reserved:8;//		
	}databit;
	u16 data16;
} MX_REG_9CH;

union 
{
	struct
	{	
		u16 Sel_sld_lv_diff:8;//	
		u16 Sel_sld_source:2;//		
		u16 Sel_sld_out_wreq:1;//	
		u16 Sld_mode_16k:1;//			
		u16 Reserved:4;//
	}databit;
	u16 data16;
} MX_REG_9DH;
union 
{
	struct
	{	
		u16 Sld_indicator:11;//	
		u16 Fg_sld_hold:1;//		
		u16 Reserved:4;//	
	}databit;
	u16 data16;
} MX_REG_9EH;

union 
{
	struct
	{	
		u16 Ad_eq_lpf_status:1;//	
		u16 Ad_eq_bpf1_status:1;//		
		
		u16 Ad_eq_bpf2_status:1;//	
		u16 Ad_eq_bpf3_status:1;//	
		
		u16 Ad_eq_bpf4_status:1;//		
		u16 Ad_eq_hpf1_status:1;//	
		
		u16 Reserved:8;//	
		u16 ad_eq_param_update:1;//		
		u16 Reserved2:1;//	
		
		
	}databit;
	u16 data16;
} MX_REG_AEH;

union 
{
	struct
	{	
		u16 Ad_eq_lpf_en:1;//	
		u16 Ad_eq_bpf1_en:1;//		
		
		u16 Ad_eq_bpf2_en:1;//	
		u16 Ad_eq_bpf3_en:1;//	
		
		u16 ad_eq_bpf4_en:1;//		
		u16 ad_eq_hpf1_en:1;//	
		
		u16 Reserved:1;//	
		u16 ad_eq_hpf1_tpy:1;//	
    u16 ad_eq_lpf_tpy:1;//			
		u16 Reserved2:7;//	
		
		
	}databit;
	u16 data16;
} MX_REG_AFH;


union 
{
	struct
	{	
		u16 Reserved:1;//	
		u16 Eq_biquad_wclr:1;//		
		
		u16 Da_eq_bpf2_status:1;//	
		u16 Da_eq_bpf3_status:1;//	
		
		u16 Da_eq_bpf4_status:1;//		
		u16 Da_eq_hpf1_status:1;//	
		
		u16 Da_eq_hpf2_status:1;//	
		u16 Da_eq_lpf1_status:1;//	
    u16 reserved2:6;//			
		u16 Da_eq_param_update:1;//	
		u16 reserved3:1;//		
		
	}databit;
	u16 data16;
} MX_REG_B0H;

union 
{
	struct
	{	
		u16 Reserved:1;//	
		u16 Eq_biquad_en:1;//		
		
		u16 Da_eq_bpf2_en:1;//	
		u16 Da_eq_bpf3_en:1;//	
		
		u16 Da_eq_bpf4_en:1;//		
		u16 Da_eq_hpf1_en:1;//	
		
		u16 Da_eq_hpf2_en:1;//	
		u16 Da_eq_lpf1_en:1;//	
    u16 Da_eq_hpf1_tpy_l:1;//			
		u16 Da_eq_hpf1_tpy_r:1;//	
		u16 Reserved2:2;//	
		u16 Da_eq_lpf1_tpy_l:1;//	
		u16 Da_eq_lpf1_tpy_r:1;//	
		u16 Reserved3:2;//
		
	}databit;
	u16 data16;
} MX_REG_B1H;


union 
{
	struct
	{	
		u16 Alc_thmin:6;//	
		u16 Alc_thmin_fast_rc_en:1;//			
		u16 Reserved:9;//		
	}databit;
	u16 data16;
} MX_REG_B2H;

union 
{
	struct
	{	
		u16 alc_bk_gain_r:6;//	
		u16 alc_ft_boost:6;//			
		u16 Alc_noise_gate_ht:3;//		
		u16 reserved:1;//			
	}databit;
	u16 data16;
} MX_REG_B3H;



union 
{
	struct
	{	
		u16 sel_rc_rate:5;//	
		u16 Drc_agc_rate_sel:3;//			
		u16 sel_drc_agc_atk:5;//		
		u16 update_drc_agc_param:1;//	
    u16 sel_drc_agc:2;//	
		
	}databit;
	u16 data16;
} MX_REG_B4H;

union 
{
	struct
	{	
		u16 noise_gate_ratio_sel:2;//	
		u16 Reserved:2;//			
		u16 Alc_noise_gate_drop_en:1;//		
		u16 Sel_ratio:2;//	
    u16 En_drc_agc_compress:1;//	
		u16 sel_drc_agc_post_bst:6;//	
    u16 Alc_drc_ratio_sel2:2;//			
	}databit;
	u16 data16;
} MX_REG_B5H;

union 
{
	struct
	{	
		u16 sel_drc_agc_noise_th:5;//	
		u16 En_drc_agc_noise_gate_hold:1;//			
		u16 en_drc_agc_noise_gate:1;//		
		u16 Reserved:5;//	
    u16 Noise_gate_boost:4;//				
	}databit;
	u16 data16;
} MX_REG_B6H;

union 
{
	struct
	{	
		u16 alc_thmax:6;//	
		u16 alc_thmax2:6;//			
		u16 Reserved:4;//						
	}databit;
	u16 data16;
} MX_REG_B7H;

union 
{
	struct
	{	
		u16 reserved:2;//	
		u16 polarity_jd_tri_lout1:1;//			
		u16 en_jd_lout1:1;//		
    u16 reserved2:2;//
		u16 polarity_jd_tri_pdm1_r:1;//	
		u16 en_jd_pdm1_r:1;//	
		
		u16 polarity_jd_tri_pdm1_l:1;//		
		u16 en_jd_pdm1_l:1;//	
		
		u16 polarity_jd_tri_hpo:1;//			
		u16 en_jd_hpo:1;//		

		u16 reserved3:1;//	
		u16 sel_gpio_jd1:3;//			
			
	}databit;
	u16 data16;
} MX_REG_BBH;

union 
{
	struct
	{	
		u16 reserved:6;//	
		u16 inv_jd3:1;//			
		u16 en_jd3_sticky:1;//		
    u16 en_irq_jd3:1;//
		u16 sel_gpio_jd2:3;//	
		u16 polarity_jd_tri_pdm2_r:1;//	
		
		u16 en_jd_pdm2_r:1;//		
		u16 polarity_jd_tri_pdm2_l:1;//		
		u16 en_jd_pdm2_l:1;//	
					
	}databit;
	u16 data16;
} MX_REG_BCH;


union 
{
	struct
	{	
		u16 reserved:1;//	
		u16 inv_jd2:1;//			
		u16 en_jd2_sticky:1;//
		
    u16 en_irq_jd2:1;//
		u16 inv_jd1_2:1;//	
		u16 en_jd1_2_sticky:1;//
		
		
		u16 en_irq_jd1_2:1;//		
		u16 inv_jd1_1:1;//		
		u16 en_jd1_1_sticky:1;//	

    u16 en_irq_jd1_1:1;//
		u16 inv_gpio_jd2:1;//	
		u16 en_gpio_jd2_sticky:1;//	
		
		u16 en_irq_gpio_jd2:1;//		
		u16 inv_gpio_jd1:1;//		
		u16 en_gpio_jd1_sticky:1;//
		u16 en_irq_gpio_jd1:1;//	
	}databit;
	u16 data16;
} MX_REG_BDH;

union 
{
	struct
	{	
		u16 reserved:2;//	
		u16 sta_gpio10:1;//			
		u16 sta_gpio_jd2:1;//
		
    u16 Reserved:4;//
		u16 Sta_micbias2_ovcd:1;//	
		u16 Sta_micbias1_ovcd:1;//
		
		
		u16 inv_micbias2_ovcd:1;//		
		u16 inv_micbias1_ovcd:1;//		
		u16 en_micbias2_ovcd_sticky:1;//	

    u16 en_micbias1_ovcd_sticky:1;//
		u16 en_irq_micbias2_ovcd:1;//	
		u16 en_irq_micbias1_ovcd:1;//	
	
	}databit;
	u16 data16;
} MX_REG_BEH;

union 
{
	struct
	{	
		u16 inv_inline:1;//	
		u16 en_inline_sticky:1;//			
		u16 sta_inline:1;//
		
    u16 en_irq_inline:1;//
		u16 sta_gpio_jd1:1;//	
		u16 sta_gpio4:1;//
		
		
		u16 sta_gpio3:1;//		
		u16 sta_gpio2:1;//		
		u16 sta_gpio1:1;//	

    u16 sta_gpio5:1;//
		u16 sta_gpio6:1;//	
		u16 sta_gpio7:1;//	
		
		
    u16 sta_jd1_1:1;//
		u16 sta_jd1_2:1;//	
		u16 sta_jd2:1;//			
		u16 sta_jd3:1;//	
		
	}databit;
	u16 data16;
} MX_REG_BFH;



union 
{
	struct
	{	
		u16 sel_gpio10_type:2;//	
		u16 reserved:2;//			
		u16 sel_gpio7_type:2;//
		
    u16 sel_gpio6_type:1;//
		u16 sel_gpio5_type:1;//	
		u16 Sel_i2s2_pin:1;//
		
		u16 reserved2:1;//		
		u16 En_serial_if:1;//		
		u16 sel_gpio4_type:1;//	

    u16 sel_gpio3_type:1;//
		u16 reserved3:1;//	
		u16 sel_gpio2_type:1;//	
		
    u16 sel_gpio1_type:1;//
				
	}databit;
	u16 data16;
} MX_REG_C0H;



union 
{
	struct
	{	
		u16 inv_gpio1:1;//	
		u16 sel_gpio1_logic:1;//			
		u16 sel_gpio1:1;//
		
    u16 inv_gpio2:1;//
		u16 sel_gpio2_logic:1;//	
		u16 sel_gpio2:1;//
		
		u16 inv_gpio3:1;//		
		u16 sel_gpio3_logic:1;//		
		u16 sel_gpio3:1;//	

    u16 inv_gpio4:1;//
		u16 sel_gpio4_logic:1;//	
		u16 sel_gpio4:1;//	
		
    u16 inv_gpio5:1;//
    u16 sel_gpio5_logic:1;//
		u16 sel_gpio5:1;//	
		u16 reserved:1;//	
		
	}databit;
	u16 data16;
} MX_REG_C1H;

union 
{
	struct
	{	
		u16 inv_gpio6:1;//	
		u16 sel_gpio6_logic:1;//			
		u16 sel_gpio6:1;//
		
    u16 inv_gpio7:1;//
		u16 sel_gpio7_logic:1;//	
		u16 sel_gpio7:1;//
		
		u16 reserved:6;//
		
		u16 inv_gpio10:1;//		
		u16 sel_gpio10_logic:1;//		
		u16 sel_gpio10:1;//	
		u16 reserved2:1;//	
		
	}databit;
	u16 data16;
} MX_REG_C2H;

union 
{
	struct
	{	
		u16 Sel_if1_adc1_data_in3:1;//	
		u16 Sel_dacl2_VAD_in:1;//			
		u16 Sel_debug_Source:2;//		
		u16 Sel_if1_adc1_txdp:1;//	
		u16 Dummy_CD:7;//			
		u16 Sel_dacr1_dac4_in:2;//	
		u16 Sel_dacl1_dac4_in:2;//		
	}databit;
	u16 data16;
} MX_REG_CDH;

union 
{
	struct
	{	
		u16 Bb_boost_gain:6;//	
		u16 Reserved:6;//			
		u16 Sel_bb_coef:3;//	
		u16 En_bb:1;//	
	}databit;
	u16 data16;
} MX_REG_CFH;

union 
{
	struct
	{	
		u16 reserved:8;//	
		u16 Mp_eg:5;//			
		u16 En_mp:1;//	
		u16 reserved2:2;//	
	}databit;
	u16 data16;
} MX_REG_D0H;

union 
{
	struct
	{	
		u16 mp_hg:6;//	
		u16 reserved:2;//			
		u16 mp_og:5;//	
		u16 mp_hp_wt:1;//	
		u16 reserved2:2;//
	}databit;
	u16 data16;
} MX_REG_D1H;


union 
{
	struct
	{	
		u16 reserved:8;//	
		u16 adj_hpf_coef_r_sel_stereo1:3;//			
		u16 Reserved:1;//	
		u16 adj_hpf_coef_l_sel_stereo1:3;//	
		u16 adj_hpf_2nd_en_stereo1:1;//
	}databit;
	u16 data16;
} MX_REG_D3H;

union 
{
	struct
	{	
		u16 adj_hpf_coef_r_num_stereo1:6;//	
		u16 reserved:2;//			
		u16 adj_hpf_coef_l_num_stereo1:6;//	
		u16 reserved2:2;//	
	}databit;
	u16 data16;
} MX_REG_D4H;

union 
{
	struct
	{	
		u16 reserved:5;//	
		u16 En_hp_amp_det:1;//			
		u16 reserved2:10;//	
	}databit;
	u16 data16;
} MX_REG_D6H;

union 
{
	struct
	{	
		u16 sel_svol:4;//	
		u16 reserved:6;//			
		u16 pow_zcd:1;//	
		
		u16 en_zcd_digital:1;//	
		u16 en_hpo_svol:1;//			
		u16 en_o_svol:1;//	

		u16 reserved2:1;//	
		u16 en_softvol:1;//			
			
	}databit;
	u16 data16;
} MX_REG_D9H;


union 
{
	struct
	{	
		u16 en_zcd_recmixl:1;//	
		u16 en_zcd_recmixr:1;//			
		u16 en_zcd_recmixm:1;//	
		
		u16 en_zcd_hpmixl:1;//	
		u16 en_zcd_hpmixr:1;//			
		u16 en_zcd_outmixl:1;//	

		u16 en_zcd_outmixr:1;//	
		u16 reserved:9;//			
			
	}databit;
	u16 data16;
} MX_REG_DAH;

union 
{
	struct
	{	
		u16 Mic_in_det_0_th:3;//	
		u16 Sel_clk_mic:2;//			
		u16 reserved:1;//		
		u16 En_inline:1;//	
		
		u16 sta_hold_down_button:1;//			
		u16 sta_double_down_button:1;//	
		u16 sta_one_down_button:1;//	
		u16 sta_hold_center_button:1;//		
		u16 sta_double_center_button:1;//			

		u16 sta_one_center_button:1;//			
		u16 sta_hold_up_button:1;//	
		u16 sta_double_up_button:1;//		
		u16 sta_one_up_button:1;//	
		
	}databit;
	u16 data16;
} MX_REG_DBH;



union 
{
	struct
	{	
		u16 in_det_window:11;//	
		u16 Conti_hold_down:1;//			
		u16 Conti_hold_center:1;//		
		u16 Conti_hold_up:1;//	
		
		u16 sel_inline_ctl_if:1;//			
		u16 en_inline_vol:1;//	
	
	}databit;
	u16 data16;
} MX_REG_DCH;


union 
{
	struct
	{	
		u16 Mic_in_det_2_th:3;//	
		u16 Mic_in_det_1_th:3;//			
		u16 Reserved:10;//	
		
	}databit;
	u16 data16;
} MX_REG_DDH;

union 
{
	struct
	{	
		u16 Aec_cmd_start:1;//			
		u16 Sel_dsp_addr_len:1;//			
		u16 Sel_dsp_data_len:2;//	
		u16 Sel_dsp_cmd:1;//			
		u16 Aec_busy:1;//		
		u16 dsp_clk_sel:2;//			
		u16 Aec_cmd:8;//
		
	}databit;
	u16 data16;
} MX_REG_E0H;
union 
{
	struct
	{			
		u16 Aec_addr:16;//			
	}databit;
	u16 data16;
} MX_REG_E1H;

union 
{
	struct
	{			
		u16 Aec_write_data:16;//			
	}databit;
	u16 data16;
} MX_REG_E2H;

union 
{
	struct
	{			
		u16 Aec_write_data2:8;//	
    u16 Reserved:8;//	
		
	}databit;
	u16 data16;
} MX_REG_E3H;


union 
{
	struct
	{			
    u16 read_data_aec:16;//		
	}databit;
	u16 data16;
} MX_REG_E4H;

union 
{
	struct
	{			
    u16 read_data_aec2:8;//
		u16 Reserved:8;//
	}databit;
	u16 data16;
} MX_REG_E5H;

union 
{
	struct
	{			
    u16 reserved:8;//8	
		
		u16 adj_hpf_coef_r_sel_mono:3;//	
    u16 reserved2:1;//	
		
		u16 adj_hpf_coef_l_sel_mono:3;//	
    u16 adj_hpf_2nd_en_mono:1;//	
			
	}databit;
	u16 data16;
} MX_REG_ECH;

union 
{
	struct
	{
    u16 adj_hpf_coef_r_num_mono:6;//	
    u16 reserved:2;//	
    u16 adj_hpf_coef_l_num_mono:6;//			
    u16 reserved2:2;//			
	}databit;
	u16 data16;
} MX_REG_EDH;

union 
{
	struct
	{
    u16 reserved:8;//0	
    u16 adj_hpf_coef_r_sel_stereo2:3;//2h	
    u16 reserved2:1;//0			
    u16 adj_hpf_coef_l_sel_stereo2:3;//	2h
    u16 adj_hpf_2nd_en_stereo2:1;//1		
	}databit;
	u16 data16;
} MX_REG_EEH;

union 
{
	struct
	{
    u16 adj_hpf_coef_r_num_stereo2:6;//	
    u16 reserved:2;//	
    u16 adj_hpf_coef_l_num_stereo2:6;//			
    u16 reserved2:2;//	
	}databit;
	u16 data16;
} MX_REG_EFH;


union 
{
	struct
	{
    u16 Sel_jd_trigger_hpo:3;//	
    u16 Sel_jd_trigger_cbj:3;//	
    u16 polarity_jd_tri_cbj:1;//			
    u16 en_jd_combo_jack:1;//	
		u16 reserved:8;//		
	}databit;
	u16 data16;
} MX_REG_F8H;


union 
{
	struct
	{
    u16 reserved:3;//	
    u16 Sel_jd_trigger_lout1:3;//	
    u16 reserved2:3;//			
    u16 Sel_jd_trigger_pdm1:3;//	
		u16 Sel_jd_trigger_pdm2:3;//
		u16 reserved3:1;//	
	}databit;
	u16 data16;
} MX_REG_F9H;

union 
{
	struct
	{
		u16 digital_gate_ctrl:1;//	
		u16 en_amp_detect_plus:1;		
    u16 reserved:1;//	
    u16 En_detect_clk_sys:1;//			
    u16 Temp_capless_bias_ctrl:3;//			
    u16 temp_cp_bias_ctrl:2;//		
		u16 Sel_rxdp_in_lr:1;//
		u16 Sel_if1_adc2_data_in1:1;//		
    u16 Sel_if1_adc1_data_in2:1;//
    u16 Sel_if1_adc1_data_in1:1;//		
		u16 Rst_dsp:1;//	
		u16 Sel_pdm2_pi2c:1;//		
		u16 Sel_pdm1_pi2c:1;//				
	}databit;
	u16 data16;
} MX_REG_FAH;

union 
{
	struct
	{
		u16 En_cbj_tie_gr:1;//	
    u16 En_cbj_tie_gl:1;//	
    u16 En_pow_pdm1_ctrl:1;//		
    u16 Sel_asrcin_clkout:1;//		
    u16 MD_PD:1;//			
		u16 Digital_gate_pad_ldo:1;//		
		u16 Sel_alc_zero_hp_time:2;//	
    u16 En_sel_in_r:1;//			
		u16 En_sel_in_l:1;//		
		u16 Sel_pow_capless:1;//
		u16 Sel_cbj_mode:2;//	
    u16 Sel_alc_zero_hp_mode:1;//		
		u16 Sel_gpio7_out:1;//	
		u16 Sel_adcdat1:1;//		
		u16 En_alc_zero_pow_hp:1;//		
	}databit;
	u16 data16;
} MX_REG_FBH;

union 
{
	struct
	{
		u16 temp_dac1_bias_ctrl:2;//	
    u16 Sel_pow_micbias2:1;//	
    u16 Auto_turnoff_micbias:1;//		
    u16 Sel_reset_cbj:1;//		
    u16 Manual_reset_cbj:1;//			
		u16 temp_psv_dac:1;//		
		u16 Sel_i2s3_data_mode:1;//		
    u16 Sel_pow_jd_debounce_source2:2;//			
		u16 Sel_irq_type:1;//				
		u16 Sel_tdm_data_mode:1;//		
		u16 Sel_i2s2_data_mode:1;//			
    u16 Sel_tdm_rx_data:1;//		
		u16 Sel_pow_jd_debounce_source1:2;//		
	}databit;
	u16 data16;
} MX_REG_FCH;

union 
{
	struct
	{
		u16 En_cbj_tie_gr:1;//	
    u16 En_cbj_tie_gl:1;//	
    u16 En_pow_pdm1_ctrl:1;//		
    u16 Sel_asrcin_clkout:1;//		
    u16 MD_PD:1;//			
		u16 Digital_gate_pad_ldo:1;//		
		u16 Sel_alc_zero_hp_time:2;//	
    u16 En_sel_in_r:1;//			
		u16 En_sel_in_l:1;//		
		u16 Sel_pow_capless:1;//
		u16 Sel_cbj_mode:2;//	
    u16 Sel_alc_zero_hp_mode:1;//		
		u16 Sel_gpio7_out:1;//	
		u16 Sel_adcdat1:1;//		
		u16 En_alc_zero_pow_hp:1;//		
	}databit;
	u16 data16;
} MX_REG_FDH;


union 
{
	struct
	{
    u16 reserved:8;//	
    u16 En_psv_dac:1;//	
    u16 En_ckgen_dac:1;//			
    u16 Ckxen_dac:1;//	
		u16 reserved2:1;//
		u16 En_ckgen_adc:1;//		
    u16 reserved3:3;//		
	}databit;
	u16 data16;
} PR_REG_3DH;

///DSP FUNCTION CONTROL////////////////////////////////////////////////////////
union 
{
	struct
	{
		u16 bypass_pitch_prepare_aecref:1;//	
    u16 bypass_ne_pitch:1;//	
    u16 bypass_pitch_prepare_LINEIN:1;//			
    u16 bypass_time_domain_AEC:1;//		
    u16 Revert_MIC0MIC1_LAEC_output:1;//					
		u16 bypass_DOA_search_ARSP_EST_proc:1;//			
		u16 reserved:7;//		
		u16 Linein_direct_out:1;//for downlink bypass	
		u16 downlink_bypass:1;//must turn on together with D13 to save mips		
		u16 bypass_all_signal_processing:1;//		
	}databit;
	u16 data16;
} DSP_REG_22FCH;
union 
{
	struct
	{
		u16 reserved:3;//			
    u16 Stress_Ratio_SPK_DRC:1;//	
    u16 common_SPK_DRC:1;//			
    u16 reserved2:1;//			
    u16 SPK_HARMONIC_GENERATION:1;//		
		u16 reserved3:1;//			
		u16 frequency_domain_AEC:1;//		
    u16 post_filter:1;//			
		u16 beam_forming:1;//		
		u16 echo_ase_proc_under_BF:1;//		
		u16 reserved4:3;//			
    u16 bypass_frequency_domain_process:1;//			
	}databit;
	u16 data16;
} DSP_REG_2303H;//Flags for doing frame processing 0x0710
union 
{
	struct
	{
		u16 adjustable_Line_out_HPF:1;//default is 180Hz HPF for 8kHz sample rate	
    u16 Mic_in_HPF_Enable:1;//80Hz	
    u16 mic_pre_emphasis_filter:1;//		
    u16 mic_de_emphasis_filter:1;//			
    u16 Line_outHPF_filter:1;//120hz 		
		u16 Line_out_HPF_Enable:1;//			
		u16 LINEIN_EMPHFILTER:1;//		
    u16 LINEIN_DEEMPHFILTER:1;//			
		u16 Spk_out_HPF_Enable:1;//		
		u16 Line_in_HPF_Enable:1;//80Hz		
		u16 reserved:4;//	
    u16 FENS_FFTONLY:1;//		
		u16 FFP:1;//		
	}databit;
	u16 data16;
} DSP_REG_2304H;//sp_flag 0x0323
union 
{
	struct
	{
		u16 reserved:4;//	
    u16 BVE:1;//	
    u16 reserved2:5;//			
    u16 BAND_WIDTH_EXT1:1;// line-in/line-out 8k, mic/spkout 16k		
    u16 BAND_WIDTH_EXT2:1;//line-in duplicated by host, linein/spkout 16k. BWE2 can be on as auxiliary shaping on 4K~8K forBWE1.			
		u16 SHORT_DELAY_WIDE_BAND:1;//		
		u16 Side_tone_Generation:1;//	
    u16 reserved3:1;//		
		u16 AVC:1;//			
	}databit;
	u16 data16;
} DSP_REG_2305H;//ft_flag 0x0000 for HF mode

u16 testdata2=0;

int rt5670Init2()
{
	int i;
	int res = 0;
	
	i2c = new mbed::I2C(SDA_PIN, GPIO_PIN23);
	I2sInit();

	for(i=0; i <50000; ++i);
	for(i=0; i <50000; ++i);
	for(i=0; i <50000; ++i);
	for(i=0; i <50000; ++i);
	for(i=0; i <50000; ++i);
	for(i=0; i <50000; ++i);

	
}


int rt5670Init3()
{
	int i;
	int res = 0;
	
	i2c = new mbed::I2C(SDA_PIN, GPIO_PIN23);
	I2sInit();
	
	for(i=0; i <50000; ++i);
	for(i=0; i <50000; ++i);
	for(i=0; i <50000; ++i);
	for(i=0; i <50000; ++i);
	for(i=0; i <50000; ++i);
	for(i=0; i <50000; ++i);

	rt5670WriteReg(0x00, 0x0000);
	
	for(i=0; i <10000; ++i);

	///////////////////////adc path agc////////////////////////////////////////////////////
	MX_REG_B4H.databit.sel_rc_rate=0xc;//Select the track clock source for DSP,0000b: clk_sys
	MX_REG_B4H.databit.Drc_agc_rate_sel=0;//
	MX_REG_B4H.databit.sel_drc_agc_atk=2;//I2S_Pre_Div5,001b:/ 2
	MX_REG_B4H.databit.update_drc_agc_param=1;//
	MX_REG_B4H.databit.sel_drc_agc=3;//I2S_Pre_Div4001b:/ 2
	//rt5670WriteReg(0xb4, MX_REG_B4H.data16);	

	////////////////////////////////////////////////////////////////////////////////////////


	////analog path cofig adc path/////////////////////////////////////////////////////	
	//MX-0Ah: IN1 Port Control - 1
	//rt5670WriteReg(0x0a, 0x6021);
	MX_REG_0AH.databit.Reserved2=1;//1h,Reserved	
	MX_REG_0AH.databit.En_bst1=0;//Combo Jack Function Control ,IN1 Port Enable Control,0b: Disable,1b: Enable
	MX_REG_0AH.databit.Reserved=4;//0 Reserved,here =4?????	
	MX_REG_0AH.databit.Sel_bst1=1+1-0+0+1;//30db,//IN1 Boost Control (BST1),2--26DB????
	rt5670WriteReg(0x0A, MX_REG_0AH.data16);

	//MX0B Combo Jack Control 2
	//res |= rt5670WriteReg(0x0b, 0x20A7);
	MX_REG_0BH.databit.Reserved3=0x27;//0x27,Reserved	
	MX_REG_0BH.databit.Reg_mode=1;//IN1 Port Mode Control 1b: Manual mode	
	MX_REG_0BH.databit.Reserved2=0;//0 Reserved
	MX_REG_0BH.databit.Capless_gat_en=0;//Capless Power Gating with IN1 Control 0b: Register control
	MX_REG_0BH.databit.Manual_tri_in1=0;////Manual Trigger For IN1 Port 0b: Low trigger
	MX_REG_0BH.databit.Reserved=4;//0 Reserved,here =1??
	rt5670WriteReg(0x0B, MX_REG_0BH.data16);	

	//MX0D IN2 Control
	//res |= rt5670WriteReg(0x0d, 0x0008);	
	MX_REG_0DH.databit.Reserved3=0x8;//8h,Reserved		
	MX_REG_0DH.databit.En_in2_df=0;//Enable IN2 Differential Input 0 Single-end
	MX_REG_0DH.databit.Reserved2=0;//0 Reserved	
	//MX_REG_0DH.databit.Sel_bst2=0;//IN2 Boost Control (BST2) Bypass
	MX_REG_0DH.databit.Sel_bst2=0;//IN2 Boost Control (BST2) Bypass
	//MX_REG_0DH.databit.Reserved=3;//Reserved ???
	MX_REG_0DH.databit.Reserved=0;//Reserved ???
	rt5670WriteReg(0x0D, MX_REG_0DH.data16);	


	//MX0E IN3 Control
	//res |= rt5670WriteReg(0x0e, 0x3000);	
	MX_REG_0EH.databit.Reserved=0;//0h Reserved
	MX_REG_0EH.databit.Sel_bst3=2-0+0+1;//???,?????????
	;//IN3 Boost Control (BST3) 30db,2--24DB
	rt5670WriteReg(0x0E, MX_REG_0EH.data16);	

	//MX-3Bh: RECMIXL Control 1
	MX_REG_3BH.databit.reserved=0;//
	MX_REG_3BH.databit.gain_bst2_recmixl=0;////000b: 0dB 001b: -3dB 010b: -6dB 011b: -9dB 100b: -12dB 101b: -15dB 110b: -18dB
	MX_REG_3BH.databit.gain_bst3_recmixl=1;//2///000b: 0dB 001b: -3dB 010b: -6dB 011b: -9dB 100b: -12dB 101b: -15dB 110b: -18dB/
	MX_REG_3BH.databit.reserved2=0;//	
	MX_REG_3BH.databit.gain_inl_recmixl=0;//000b: 0dB 001b: -3dB 010b: -6dB 011b: -9dB 100b: -12dB 101b: -15dB 110b: -18dB
	MX_REG_3BH.databit.reserved3=0;//	
	rt5670WriteReg(0x3B, MX_REG_3BH.data16);		

	//MX-3Ch: RECMIXL Control 2
	//res |= rt5670WriteReg(0x3c, 0x007D); /* BST1 */
	MX_REG_3CH.databit.reserved=1;//
	MX_REG_3CH.databit.Mu_bst1_recmixl=1;//
	MX_REG_3CH.databit.Mu_bst2_recmixl=1;//
	MX_REG_3CH.databit.Mu_bst3_recmixl=0;//	
	MX_REG_3CH.databit.reserved2=1;//
	MX_REG_3CH.databit.Mu_inl_recmixl=1;//	
	MX_REG_3CH.databit.reserved3=1;//	
	MX_REG_3CH.databit.gain_bst1_recmixl=0;//000b: 0dB 001b: -3dB 010b: -6dB 011b: -9dB 100b: -12dB 101b: -15dB 110b: -18dB
	rt5670WriteReg(0x3C, MX_REG_3CH.data16);	

	//MX-3Dh: RECMIXR Control 1
	MX_REG_3DH.databit.reserved=0;//
	MX_REG_3DH.databit.gain_bst2_recmixr=0;////000b: 0dB 001b: -3dB 010b: -6dB 011b: -9dB 100b: -12dB 101b: -15dB 110b: -18dB
	MX_REG_3DH.databit.gain_bst3_recmixr=0;///000b: 0dB 001b: -3dB 010b: -6dB 011b: -9dB 100b: -12dB 101b: -15dB 110b: -18dB/
	MX_REG_3DH.databit.reserved2=0;//	
	MX_REG_3DH.databit.gain_inr_recmixr=0;//000b: 0dB 001b: -3dB 010b: -6dB 011b: -9dB 100b: -12dB 101b: -15dB 110b: -18dB
	MX_REG_3DH.databit.reserved3=0;//	
	rt5670WriteReg(0x3D, MX_REG_3DH.data16);		
	//MX-3Eh: RECMIXR Control 2
	//res |= rt5670WriteReg(0x3e, 0x0077); 
	MX_REG_3EH.databit.reserved=1;//
	MX_REG_3EH.databit.Mu_bst1_recmixr=0;//
	MX_REG_3EH.databit.Mu_bst2_recmixr=1;//
	MX_REG_3EH.databit.Mu_bst3_recmixr=1;//	
	MX_REG_3EH.databit.reserved2=1;//
	MX_REG_3EH.databit.Mu_inr_recmixr=1;//	
	MX_REG_3EH.databit.reserved3=1;//
	MX_REG_3EH.databit.gain_bst1_recmixr=1;//2//000b: 0dB 001b: -3dB 010b: -6dB 011b: -9dB 100b: -12dB 101b: -15dB 110b: -18dB	
	rt5670WriteReg(0x3E, MX_REG_3EH.data16);	
	//MX-3Fh: RECMIXM Control 1
	MX_REG_3FH.databit.reserved=0;//
	MX_REG_3FH.databit.gain_bst2_recmixr=0;////000b: 0dB 001b: -3dB 010b: -6dB 011b: -9dB 100b: -12dB 101b: -15dB 110b: -18dB
	MX_REG_3FH.databit.gain_bst3_recmixr=0;///000b: 0dB 001b: -3dB 010b: -6dB 011b: -9dB 100b: -12dB 101b: -15dB 110b: -18dB/
	MX_REG_3FH.databit.reserved2=0;//	
	rt5670WriteReg(0x3F, MX_REG_3FH.data16);	
	//MX-40h: RECMIXM Control 2
	//res |= rt5670WriteReg(0x40, 0x001b); /* BST2-ADC3 */
	MX_REG_40H.databit.reserved=1;//
	MX_REG_40H.databit.Mu_bst1_recmixm=1;//
	MX_REG_40H.databit.Mu_bst2_recmixm=0;//
	MX_REG_40H.databit.Mu_bst3_recmixm=1;//	
	MX_REG_40H.databit.reserved2=1;//
	MX_REG_40H.databit.gain_bst1_recmixm=0;//000b: 0dB 001b: -3dB 010b: -6dB 011b: -9dB 100b: -12dB 101b: -15dB 110b: -18dB	
	rt5670WriteReg(0x40, MX_REG_40H.data16);	
	//////////////////////////////////////////////////////////////////////////

	//MX1C Stereo1 ADC Digital Volume Control
	//res |= rt5670WriteReg(0x2d, 0x6003);///////////////////////////////////////////////////////////////// 
	MX_REG_1CH.databit.Ad_gain_r=0x2F;//=0DB with 0.375dB/Step
	MX_REG_1CH.databit.Mu_adc_vol_r=0;//=0 
	MX_REG_1CH.databit.Ad_gain_l=0X2F;//=0DB with 0.375dB/Step
	MX_REG_1CH.databit.Mu_adc_vol_l=0;//=0 
	rt5670WriteReg(0x1C, MX_REG_1CH.data16);	
	// MX1D Mono ADC Digital Volume Control
	MX_REG_1DH.databit.Mono_ad_gain_r=0x2F;//=0DB with 0.375dB/Step
	MX_REG_1DH.databit.reserved=0;//=0 
	MX_REG_1DH.databit.Mono_ad_gain_l=0X2F;//=0DB with 0.375dB/Step
	MX_REG_1DH.databit.reserved2=0;//=0 
	rt5670WriteReg(0x1D, MX_REG_1DH.data16);

	//MX1E ADC Boost Gain Control 1
	MX_REG_1EH.databit.reserved=0;//
	MX_REG_1EH.databit.Stereo2_ad_comp_gain=0;//=00b: 0dB 01b: 1dB 10b: 2dB 11b: 3dB
	MX_REG_1EH.databit.Stereo2_ad_boost_gain_r=0;//=0DB 00b: 0dB 01b: 12dB 10b: 24dB 11b: 36dB	
	MX_REG_1EH.databit.Stereo2_ad_boost_gain_l=0;//=00b: 0dB 01b: 12dB 10b: 24dB 11b: 36dB	
	MX_REG_1EH.databit.Stereo1_ad_comp_gain=0;//00b: 0dB 01b: 1dB 10b: 2dB 11b: 3dB
	MX_REG_1EH.databit.Stereo1_ad_boost_gain_r=0;//=0 00b: 0dB 01b: 12dB 10b: 24dB 11b: 36dB	
	MX_REG_1EH.databit.Stereo1_ad_boost_gain_l=0;//=00b: 0dB 01b: 12dB 10b: 24dB 11b: 36dB	
	rt5670WriteReg(0x1E, MX_REG_1EH.data16);
	// MX1F Stereo2 ADC Digital Volume Control	
	MX_REG_1FH.databit.Ad2_gain_r=0x2F;//=0DB with 0.375dB/Step
	MX_REG_1FH.databit.Mu_adc2_vol_r=0;//=0 
	MX_REG_1FH.databit.Ad2_gain_l=0X2F;//=0DB with 0.375dB/Step
	MX_REG_1FH.databit.Mu_adc2_vol_l=0;//=0 
	rt5670WriteReg(0x1F, MX_REG_1FH.data16);	


	////digital path cofig digital adc path/////////////////////////////////////////////////////	
	//MX26 Stereo2 ADC Mixer control
	//res |= rt5670WriteReg(0x26, 0x3c20);////////////////////////////////////////////
	MX_REG_26H.databit.reserved=0;//0h Reserved	
	MX_REG_26H.databit.mu_stereo2_adcr2=1;//MX26[5]=1
	MX_REG_26H.databit.mu_stereo2_adcr1=0;//MX26[6]=0	
	MX_REG_26H.databit.reserved2=0;//
	MX_REG_26H.databit.Sel_stereo2_dmic=0;//MX26[9-8]=0 	
	MX_REG_26H.databit.Sel_stereo2_adc=1;//MX26[10]=1	
	MX_REG_26H.databit.sel_stereo2_adc2=1;//MX26[11]=1 	
	MX_REG_26H.databit.sel_stereo2_adc1=1;//MX26[12]=1	
	MX_REG_26H.databit.mu_stereo2_adcl2=1;//MX26[13]=1 
	MX_REG_26H.databit.mu_stereo2_adcl1=0;//MX26[14]=0	
	MX_REG_26H.databit.Sel_stereo2_lr_mix=0;//MX26[15]=0 
	rt5670WriteReg(0x26, MX_REG_26H.data16);	

	//MX27 Stereo1 ADC Mixer control
	//res |= rt5670WriteReg(0x27, 0x3820); //////////////////////////////////////////
	MX_REG_27H.databit.Sel_dmic3_data=0;//0h UNCARE
	MX_REG_27H.databit.reserved=0;//0h UNCARE
	MX_REG_27H.databit.mu_stereo1_adcr2=1;//0h MX[5]=1	RIGHT CHAN
	MX_REG_27H.databit.mu_stereo1_adcr1=0;//0h MX[6]=0  RIGHT CHAN	
	MX_REG_27H.databit.reserved2=0;//0h UNCARE
	MX_REG_27H.databit.Sel_stereo1_dmic=0;//h MX[9:8]=0 UNCARE
	MX_REG_27H.databit.sel_stereo1_adc2=1;//h MX[11]=1 UNCARE		
	MX_REG_27H.databit.Sel_stereo1_adc=0; //h MX[10]=0 ADC1 => Left channel  ADC2 => Right channel
	MX_REG_27H.databit.sel_stereo1_adc1=1;//h MX[12]=1 Select Stereo1 ADC L/R Channel Source 1:ADC1	
	MX_REG_27H.databit.mu_stereo1_adcl2=1;//h MX[13]=1 LEFT CHAN
	MX_REG_27H.databit.mu_stereo1_adcl1=0;//h MX[14]=0 LEFT CHAN	
	MX_REG_27H.databit.reserved3=0;//0h 
	rt5670WriteReg(0x27, MX_REG_27H.data16);		

	//MX28 Mono ADC Mixer control
	//res |= rt5670WriteReg(0x28, 0x3c71); //////////////////////////////
	MX_REG_28H.databit.Sel_mono_dmic_r=1;//MX28[1-0]=1
	MX_REG_28H.databit.sel_mono_adc_r=0;//MX28[2]=0
	MX_REG_28H.databit.sel_mono_adcr2=0;//MX28[3]=0
	MX_REG_28H.databit.sel_mono_adcr1=1;//MX28[4]=1
	MX_REG_28H.databit.mu_mono_adcr2=1;//MX28[5]=1
	MX_REG_28H.databit.mu_mono_adcr1=0;//MX28[6]=0	
	MX_REG_28H.databit.Reserved=0;//=0 
	MX_REG_28H.databit.Sel_mono_dmic_l=0;//MX28[9-8]=0	
	MX_REG_28H.databit.sel_mono_adc_l=0; //h MX[10]=1 
	MX_REG_28H.databit.sel_mono_adcl2=0;//h MX[11]=1 
	MX_REG_28H.databit.sel_mono_adcl1=1;//h MX[12]=1 
	MX_REG_28H.databit.mu_mono_adcl2=1;//h MX[13]=1 
	MX_REG_28H.databit.mu_mono_adcl1=0;//MX[14]=0 0h 
	MX_REG_28H.databit.reserved=0;//0h 
	rt5670WriteReg(0x28, MX_REG_28H.data16);
	//////////////////////////////////////////////////////////////////////////////////

	////////digital DAC/////////////////////////////////////////////////////////////
	MX_REG_19H.databit.vol_dac1_r=0XAF;//
	MX_REG_19H.databit.vol_dac1_l=0XAF;//
	rt5670WriteReg(0x19, MX_REG_19H.data16);	

	MX_REG_1AH.databit.vol_dac2_r=0XAF;//
	MX_REG_1AH.databit.vol_dac2_l=0XAF;//
	rt5670WriteReg(0x1A, MX_REG_1AH.data16);
		
	MX_REG_29H.databit.reserved=0;//
	MX_REG_29H.databit.Mu_dac1_r=1;//
	MX_REG_29H.databit.Mu_stereo1_adc_mixer_r=0;//
	MX_REG_29H.databit.Sel_dacl1=0;//
	MX_REG_29H.databit.Sel_dacr1=0;//
	MX_REG_29H.databit.reserved2=0;//
	MX_REG_29H.databit.Mu_dac1_l=1;//	
	MX_REG_29H.databit.Mu_stereo1_adc_mixer_l=0;//	
	rt5670WriteReg(0x29, MX_REG_29H.data16);	

	MX_REG_2AH.databit.gain_dacl1_to_stereo_r=0;//
	MX_REG_2AH.databit.mu_stereo_dacl1_mixr=1;//
	MX_REG_2AH.databit.Mu_snc_to_dac_r=1;//
	MX_REG_2AH.databit.gain_dacr2_to_stereo_r=0;//
	MX_REG_2AH.databit.mu_stereo_dacr2_mixr=1;//
	MX_REG_2AH.databit.gain_dacr1_to_stereo_r=0;//
	MX_REG_2AH.databit.mu_stereo_dacr1_mixr=0;//
	MX_REG_2AH.databit.reserved=0;//
	MX_REG_2AH.databit.gain_dacr1_to_stereo_l=0;//
	MX_REG_2AH.databit.mu_stereo_dacr1_mixl=1;//
	MX_REG_2AH.databit.Mu_snc_to_dac_l=1;//
	MX_REG_2AH.databit.gain_dacl2_to_stereo_l=0;//
	MX_REG_2AH.databit.mu_stereo_dacl2_mixl=1;//
	MX_REG_2AH.databit.gain_dacl1_to_stereo_l=0;//
	MX_REG_2AH.databit.mu_stereo_dacl1_mixl=0;//
	MX_REG_2AH.databit.reserved2=0;//
	rt5670WriteReg(0x2A, MX_REG_2AH.data16);

	MX_REG_2BH.databit.reserved=0;//
	MX_REG_2BH.databit.gain_mono_r_dacl2=0;//
	MX_REG_2BH.databit.mu_mono_dacl2_mixr=1;//
	MX_REG_2BH.databit.gain_mono_r_dacr2=0;//
	MX_REG_2BH.databit.mu_mono_dacr2_mixr=0;//
	MX_REG_2BH.databit.gain_mono_r_dacr1=0;//
	MX_REG_2BH.databit.mu_mono_dacr1_mixr=1;//
	MX_REG_2BH.databit.reserved2=0;//
	MX_REG_2BH.databit.gain_mono_l_dacr2=0;//
	MX_REG_2BH.databit.mu_mono_dacr2_mixl=1;//
	MX_REG_2BH.databit.gain_mono_l_dacl2=0;//
	MX_REG_2BH.databit.mu_mono_dacl2_mixl=0;//
	MX_REG_2BH.databit.gain_mono_l_dacl1=0;//
	MX_REG_2BH.databit.mu_mono_dacl1_mixl=1;//	
	MX_REG_2BH.databit.reserved3=0;//
	rt5670WriteReg(0x2B, MX_REG_2BH.data16);
	rt5670WriteReg(0x18, 0X018B);

	MX_REG_2CH.databit.reserved=0;//
	MX_REG_2CH.databit.gain_dacl2_to_dacmixr=0;//
	MX_REG_2CH.databit.Mu_dacl2_to_dacmixr=1;//
	MX_REG_2CH.databit.gain_dacr2_to_dacmixl=0;//
	MX_REG_2CH.databit.Mu_dacr2_to_dacmixl=0;//
	MX_REG_2CH.databit.gain_dacr2_to_dacmixr=0;//
	MX_REG_2CH.databit.Mu_dacr2_to_dacmixr=1;//
	MX_REG_2CH.databit.gain_stereomixr_to_dacmixr=0;//
	MX_REG_2CH.databit.Mu_stereomixr_to_dacmixr=1;//
	MX_REG_2CH.databit.gain_dacl2_to_dacmixl=0;//
	MX_REG_2CH.databit.Mu_dacl2_to_dacmixl=1;//
	MX_REG_2CH.databit.gain_stereomixl_to_dacmixl=0;//
	MX_REG_2CH.databit.Mu_stereomixl_to_dacmixl=1;//	
	rt5670WriteReg(0x2C, MX_REG_2CH.data16);

	MX_REG_2EH.databit.Vol_txdp_r=0x2f - 20+10;//
	MX_REG_2EH.databit.Reserved=0;//
	MX_REG_2EH.databit.Vol_txdp_l=0x2f - 20+10;//
	MX_REG_2EH.databit.Reserved2=0;//
	rt5670WriteReg(0x2E, MX_REG_2EH.data16);
	////////////////////////////////////////////////////////////////////////////////////////////

	MX_REG_2DH.databit.Sel_dsp_dl_bypass=0;//MX2D[0]=1--> DSP Downlink Bypass Bypass DSP
	MX_REG_2DH.databit.Sel_dsp_ul_bypass=0;//MX2D[1]=1--> DSP Uplink Bypass Bypass DSP

	MX_REG_2DH.databit.Sel_tdm_txdp_slot=0;//MX2D[3-2]=0 TXDP TDM Channel Slot Selection to Stereo Channel 00b: Slot 0/1
	MX_REG_2DH.databit.Sel_src_to_txdp=0;//MX2D[5-4]=0 Select SRC to TxDP
	MX_REG_2DH.databit.Sel_txdc_data=0;//MX2D[7-6]=00 TxDC Output Data Swap l/r
	MX_REG_2DH.databit.Sel_txdp_data=0;//MX2D[9-8]=00 TxDP Output Data Swap l/r
	MX_REG_2DH.databit.Reserved=0;//=0 
	MX_REG_2DH.databit.Sel_src_to_rxdp=0;//MX2D[12-11]=0,Select SRC to RxDP :BAPASS
	MX_REG_2DH.databit.Sel_rxdp_in=3; //h MX2D[15-13]=3,RxDP Input Selection 011b Stereo2_ADC_Mixer_L/R
	rt5670WriteReg(0x2D, MX_REG_2DH.data16);	  

	MX_REG_1BH.databit.Sel_dacr2=4;//4-->TxDP_ADC_R,3--> TxDC_DAC_R
	MX_REG_1BH.databit.Reserved=0;//
	MX_REG_1BH.databit.Sel_dacl2=5;//5-->TxDP_ADC_L,3--> TxDC_DAC_L
	MX_REG_1BH.databit.Reserved2=0;//
	MX_REG_1BH.databit.Mu_dac2_r=0;//
	MX_REG_1BH.databit.Mu_dac2_l=0;//
	MX_REG_1BH.databit.Reserved3=0;// 
	rt5670WriteReg(0x1B, MX_REG_1BH.data16);	



	//analog path cofig dac path///////////////////////////////////////////////////////////
	////Headphone Output Volume
	MX_REG_02H.databit.Vol_hpor=0x08;//HPO Right Volume (HPOR[5:0]) in 1.5 dB step	
	MX_REG_02H.databit.Reserved=0;//
	MX_REG_02H.databit.mu_hpo_r=0;//Mute Right HPO Volume
	MX_REG_02H.databit.vol_hpol=0x08;//HPO Left Volume (HPOL[5:0]) in 1.5 dB step
	MX_REG_02H.databit.Reserved2=0;//
	MX_REG_02H.databit.mu_hpo_l=0;//Mute Left HPO Volume
	rt5670WriteReg(0x02, MX_REG_02H.data16);	
	// MX03 LOUT1 Control
	MX_REG_03H.databit.Vol_outr=0x08;//Output Right Volume 1.5 dB per-step
	MX_REG_03H.databit.Reserved=0;//
	MX_REG_03H.databit.Mu_lout_r=0;//Mute LOUT 1 Right channel Control
	MX_REG_03H.databit.Vol_outl=0x08;//Output Left VolSume 1.5 dB per-step
	MX_REG_03H.databit.En_dfo1=0;//Enable Differential LOUT1 0b: Disable 1b: Enable
	MX_REG_03H.databit.Mu_lout_l=0;//Mute LOUT 1 Left channel Control
	rt5670WriteReg(0x03, MX_REG_03H.data16);
	//MX45 HPMIX Control
	MX_REG_45H.databit.Mu_dacl1_hpomixl=0;//Mute Control for DACL1 to HPMIXL
	MX_REG_45H.databit.Mu_inl1_hpomixl=1;//Mute Control for INL1 to HPMIXL
	MX_REG_45H.databit.Mu_dacr1_hpomixr=0;//Mute Control for DACR1 to HPMIXR
	MX_REG_45H.databit.Mu_inr1_hpmixr=1;//Mute Control for INR1 to HPMIXR
	MX_REG_45H.databit.reserved=0;//
	MX_REG_45H.databit.En_bst_hp=0;//Gain Control for HPOMIX 0db,1 -6db
	MX_REG_45H.databit.Mu_hpvol_hpo=0;//HPOVOL to HPO Mute Control
	MX_REG_45H.databit.Mu_dac1_hpo=1;//DAC1 to HPO Mute Control
	MX_REG_45H.databit.reserved2=1;//
	rt5670WriteReg(0x45, MX_REG_45H.data16);

	//MX4F Output Left Mixer 1 Control	
	MX_REG_4FH.databit.Mu_dacl1_outmixl1=1;//	
	MX_REG_4FH.databit.Mu_dacl2_outmixl1=0;//
	//MX_REG_4FH.databit.Mu_dacl2_outmixl1=1;//	
	MX_REG_4FH.databit.Reserved=0;//
	MX_REG_4FH.databit.Mu_inl_outmixl1=1;//
	MX_REG_4FH.databit.Mu_bst1_outmixl1=1;//
	//MX_REG_4FH.databit.Mu_bst1_outmixl1=0;//	
	MX_REG_4FH.databit.Mu_bst2_outmixl1=1;//0db,1 -6db
	MX_REG_4FH.databit.Reserved2=0;//
	rt5670WriteReg(0x4F, MX_REG_4FH.data16);

	//MX52 Output Right Mixer 1 Control
	MX_REG_52H.databit.Mu_dacr1_outmixr1=1;//
	MX_REG_52H.databit.Mu_dacr2_outmixr1=0;//
	//MX_REG_52H.databit.Mu_dacr2_outmixr1=1;//
	MX_REG_52H.databit.Reserved=0;//
	MX_REG_52H.databit.Mu_inr_outmixr1=1;//
	MX_REG_52H.databit.Reserved2=0;//
	MX_REG_52H.databit.Mu_bst3_outmixr1=1;//0db,1 -6db
	//MX_REG_52H.databit.Mu_bst3_outmixr1=0;//0db,1 -6db			
	MX_REG_52H.databit.Reserved3=1;//
	rt5670WriteReg(0x52, MX_REG_52H.data16);

	// MX53 LOUT Mixer 1/2 Control
	MX_REG_53H.databit.reserved=0;//
	MX_REG_53H.databit.bst_lout1=0;//0db
	MX_REG_53H.databit.mu_outmixr1_lout1=0;//
	MX_REG_53H.databit.mu_outmixl1_lout1=0;//
	MX_REG_53H.databit.mu_dacr1_lout1=1;//
	MX_REG_53H.databit.mu_dacl1_lout1=1;//0db,1 -6db
	rt5670WriteReg(0x53, MX_REG_53H.data16);

	//De-POP Mode Control 
	//rt5670WriteReg(0x8F, 0x3100);
	//rt5670WriteReg(0x8E, 0x8019);	
	//HPOUT charge pump
	//rt5670WriteReg(0x91, 0x0E06);	




	///////////////////////////////////////////////////////////////////////////////////	

	//INTERFACE/////////////////////////////////////////////
	//MX_REG_70H/MX_REG_71H/MX_REG_72H:I2S1/I2S2/I2S3 Digital Interface Control///////////////////////////	
	MX_REG_70H.databit.sel_i2s1_format=0;////00b: I2S format
	MX_REG_70H.databit.sel_i2s1_len=0;////00b: 16 bits
	MX_REG_70H.databit.reserved=0;//		
	MX_REG_70H.databit.Inv_i2s1_bclk=0;//	//I2S1 BCLK Polarity Control//0b: Normal
	MX_REG_70H.databit.en_i2s1_in_comp=0;////I2S1 Input Data Compress (For DACDAT1 Input)//00b: OFF	
	MX_REG_70H.databit.en_i2s1_out_comp=0;////00b: OFF
	MX_REG_70H.databit.reserved2=0;//	
	MX_REG_70H.databit.Sel_i2s1_ms=1;////I2S1 Digital Interface Mode Control//1b: Slave Mode
	rt5670WriteReg(0x70, MX_REG_70H.data16);	

	//res |= rt5670WriteReg(0x73, 0x2220);	
	MX_REG_73H.databit.sel_adc_osr=0;//ADC Over Sample Rate Select 00b: 128Fs 01b: 64Fs 10b: 32Fs 11b: Reserved
	MX_REG_73H.databit.sel_dac_osr=0;//DaC Over Sample Rate Select 00b: 128Fs 01b: 64Fs 10b: 32Fs 11b: Reserved
	MX_REG_73H.databit.sel_i2s_pre_div3=2;//I2S Pre-Divider 3 256*FS=MCLK/3 @MCLK=12.288M FS=16K //000b: /1  //001b: /2 //010b: /3 //011b: /4
	MX_REG_73H.databit.sel_i2s_bclk_ms3=0;////I2S3 Master Mode Clock Relative of BCLK and LRCK//0b: 16Bits (32FS)
	MX_REG_73H.databit.sel_i2s_pre_div2=2;////I2S Pre-Divider 2
	MX_REG_73H.databit.sel_i2s_bclk_ms2=0;//
	MX_REG_73H.databit.sel_i2s_pre_div1=2;////I2S Pre-Divider 1
	MX_REG_73H.databit.reserved=0;//
	rt5670WriteReg(0x73, MX_REG_73H.data16);	

	MX_REG_74H.databit.Reserved=0;//
	MX_REG_74H.databit.Mono_adhpf_en=1;////I2S Pre-Divider 2
	MX_REG_74H.databit.Adhpf_en=1;//Enable Stereo1/2 ADC High Pass Filter
	MX_REG_74H.databit.Dehpf_en=1;//Enable Stereo/Mono DAC High Pass Filter
	MX_REG_74H.databit.reserved=0;//Enable Mono ADC High Pass Filter
	rt5670WriteReg(0x74, MX_REG_74H.data16);	

	MX_REG_77H.databit.sel_i2s_rx_ch8=0;//0
	MX_REG_77H.databit.sel_i2s_rx_ch6=0;//
	MX_REG_77H.databit.sel_i2s_rx_ch4=0;//0
	MX_REG_77H.databit.sel_i2s_rx_ch2=0;//
	MX_REG_77H.databit.reserved2=0;//0
	MX_REG_77H.databit.rx_adc_data_sel=0;//ADC Data to ADCDAT Data Location,0b: normal
	MX_REG_77H.databit.Channel_length=3;//TDM Channel Length 3: 32bit (For Slave Mode and Master Mode)
	MX_REG_77H.databit.Tdmslot_sel=0;//TDM Channel Number Select 00b: 2ch
	MX_REG_77H.databit.mode_sel=0;//0b: Normal I2S Mode
	MX_REG_77H.databit.reserved=0;//I2S1 19.2MHz MCLK_In, Master Mode, 64FS/50FS Frame Rate Control 0b: 64FS
	rt5670WriteReg(0x77, MX_REG_77H.data16);

	MX_REG_78H.databit.mute_tdm8_outr=0;//0
	MX_REG_78H.databit.mute_tdm8_outl=0;//
	MX_REG_78H.databit.mute_tdm6_outr=0;//0
	MX_REG_78H.databit.mute_tdm6_outl=0;//
	MX_REG_78H.databit.mute_tdm4_outr=0;//0
	MX_REG_78H.databit.mute_tdm4_outl=0;//
	MX_REG_78H.databit.mute_tdm2_outr=0;//0
	MX_REG_78H.databit.mute_tdm2_outl=0;//
	MX_REG_78H.databit.reserved2=0;//
	MX_REG_78H.databit.lrck_pulse_sel=0;//LRCK Pulse Width Select (Master Mode Only) 0b: One BCLK width
	MX_REG_78H.databit.reserved=4;//100 reserved[2]: ch0/1/2/3 valid;reserved[1]: ch valid disable;reserved[0]: loopback disable
	MX_REG_78H.databit.sel_i2s_lrck_polarity=0;//
	rt5670WriteReg(0x78, MX_REG_78H.data16);

	MX_REG_79H.databit.sel_i2s_tx_r_ch4=3;//IF1_DAC2_R Data Selection:Slot3
	MX_REG_79H.databit.Reserved=0;//
	MX_REG_79H.databit.sel_i2s_tx_l_ch4=2;//IF1_DAC2_L Data Selection:Slot2
	MX_REG_79H.databit.Reserved2=0;//
	MX_REG_79H.databit.sel_i2s_tx_r_ch2=1;//IF1_DAC1_R Data Selection:Slot1
	MX_REG_79H.databit.Reserved3=0;//
	MX_REG_79H.databit.sel_i2s_tx_l_ch2=0;//IF1_DAC1_L Data Selection:Slot0
	MX_REG_79H.databit.Reserved4=0;//
	rt5670WriteReg(0x79, MX_REG_79H.data16);	

	////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////
	////power control///////////////////////////////////////////////////////////////////  
	//res |= rt5670WriteReg(0x61, 0x980E);//////////////////////////////////////////////
	MX_REG_61H.databit.reserved=0;//
	MX_REG_61H.databit.Pow_adc_r=1;//
	MX_REG_61H.databit.Pow_adc_l=1;//
	MX_REG_61H.databit.Pow_adc_3=1;// 
	MX_REG_61H.databit.reserved2=0;//
	MX_REG_61H.databit.Pow_dac_r_2=1;//
	MX_REG_61H.databit.Pow_dac_l_2=1;//
	MX_REG_61H.databit.reserved3=0;//
	MX_REG_61H.databit.Pow_dac_r_1=1;//
	MX_REG_61H.databit.Pow_dac_l_1=1;//
	MX_REG_61H.databit.En_i2s3=0;//
	MX_REG_61H.databit.En_i2s2=0;//
	MX_REG_61H.databit.En_i2s1=1;//
	rt5670WriteReg(0x61, MX_REG_61H.data16);		  
	  
	//res |= rt5670WriteReg(0x62, 0xD001);


	//res |= rt5670WriteReg(0x63, 0xf8fd);  
	MX_REG_63H.databit.Dvo_ldo=5;//Selection of the LDO output 000b: Reserved 001b: Reserved 010b: 1.0V 011b: 1.2V 100b: 1.25V 101b: 1.3V 110b: 1.35V 111b: 1.4V
	MX_REG_63H.databit.En_fastb2=1;//
	MX_REG_63H.databit.Pow_vref2=1;//
	MX_REG_63H.databit.En_amp_hp=1;// 
	MX_REG_63H.databit.En_r_hp=1;//
	MX_REG_63H.databit.En_l_hp=1;//
	MX_REG_63H.databit.reserved=0;//
	MX_REG_63H.databit.Pow_bg_bias=1;//
	MX_REG_63H.databit.Pow_lout=1;//
	MX_REG_63H.databit.Pow_main_bias=1;//
	MX_REG_63H.databit.En_fastb1=1;//
	MX_REG_63H.databit.En_fastb1=1;//
	MX_REG_63H.databit.Pow_vref1=1;//
	rt5670WriteReg(0x63, MX_REG_63H.data16);

	for(i=0; i <10000; ++i);  
	for(i=0; i <10000; ++i);
	for(i=0; i <10000; ++i);
	for(i=0; i <10000; ++i);
	for(i=0; i <10000; ++i);  
	for(i=0; i <10000; ++i);
	for(i=0; i <10000; ++i);
	for(i=0; i <10000; ++i);	

	//res |= rt5670WriteReg(0x64, 0xEC74); /* BST1 power */ 
	MX_REG_64H.databit.reserved=0;//Selection of the LDO output 000b: Reserved 001b: Reserved 010b: 1.0V 011b: 1.2V 100b: 1.25V 101b: 1.3V 110b: 1.35V 111b: 1.4V
	MX_REG_64H.databit.Pow_jd2=0;//
	MX_REG_64H.databit.Pow_jd1=1;//
	MX_REG_64H.databit.reserved2=0;// 

	MX_REG_64H.databit.Pow_bst3_2=1;//
	MX_REG_64H.databit.Pow_bst2_2=1;//
	MX_REG_64H.databit.Pow_bst1_2=1;//
	MX_REG_64H.databit.reserved3=0;//

	MX_REG_64H.databit.Pow_pll=0;//
	MX_REG_64H.databit.Pow_micbias2=1;//
	MX_REG_64H.databit.Pow_micbias1_1=1;//
	MX_REG_64H.databit.reserved4=0;//

	MX_REG_64H.databit.Pow_bst3_1=1;//
	MX_REG_64H.databit.Pow_bst2_1=1;//
	MX_REG_64H.databit.Pow_bst1_1=1;//
	rt5670WriteReg(0x64, MX_REG_64H.data16);

	//res |= rt5670WriteReg(0x65, 0x0E01);	  
	MX_REG_65H.databit.reserved=1;//
	MX_REG_65H.databit.Pow_recmixm=1;//
	MX_REG_65H.databit.Pow_recmixr=1;//

	MX_REG_65H.databit.Pow_recmixl=1;//
	MX_REG_65H.databit.reserved2=0;//
	MX_REG_65H.databit.Pow_outmixr=1;//
	MX_REG_65H.databit.Pow_outmixl=1;//
	rt5670WriteReg(0x65, MX_REG_65H.data16);	  

	MX_REG_66H.databit.Pow_micbias1_2=1;//
	MX_REG_66H.databit.Pow_micbias2_2=1;//
	MX_REG_66H.databit.reserved=0;//  
	MX_REG_66H.databit.Pow_mic_in_det=1;//
	MX_REG_66H.databit.reserved2=0;//
	MX_REG_66H.databit.Pow_inrvol=0;//
	MX_REG_66H.databit.Pow_inlvol=0;//
	MX_REG_66H.databit.Pow_hpovolr=1;//
	MX_REG_66H.databit.Pow_hpovoll=1;//
	MX_REG_66H.databit.reserved3=0;//
	rt5670WriteReg(0x66, MX_REG_66H.data16);	  
	  
	rt5670WriteReg(0x6a, 0x003D);
	rt5670WriteReg(0x6c, 0x3640);
	rt5670WriteReg(0x6a, 0x0039);
	rt5670WriteReg(0x6c, 0x3500);   



	//rt5670WriteReg(0xfa, 0xc019);
	//rt5670WriteReg(0xfb, 0x0730);
	//rt5670WriteReg(0xfc, 0x0084);
	MX_REG_FAH.databit.digital_gate_ctrl=1;//Enable gate mode with MCLK for power saving 0b: Disable
	MX_REG_FAH.databit.en_amp_detect_plus=0;//Enable auto fine tune register setting for HPO 0b: Disable (default)
	MX_REG_FAH.databit.reserved=0;//
	MX_REG_FAH.databit.En_detect_clk_sys=1;//Enable MCLK Dtection and Auto Switch to RC Clock When MCLK is Remove
	MX_REG_FAH.databit.Temp_capless_bias_ctrl=1;//Default Register Settings for HP Low Power Mode PR14[12:10]
	MX_REG_FAH.databit.temp_cp_bias_ctrl=0;//Default Register Settings for HP Low Power Mode=>PR14[15:14]
	MX_REG_FAH.databit.Sel_rxdp_in_lr=0;//Select DSP Rxdp Input Data 0b: (L+R)/2
	MX_REG_FAH.databit.Sel_if1_adc2_data_in1=0;//Selection for IF1 ADC2 Input Data 0b: IF_ADC2 or VAD_ADC
	MX_REG_FAH.databit.Sel_if1_adc1_data_in2=0;//Selection-2 for IF1 ADC1 Input Data :0b: IF_ADC1 or IF1_ADC3
	MX_REG_FAH.databit.Sel_if1_adc1_data_in1=0;//Selection-1 for IF1 ADC1 Input Data:0b: IF_ADC1
	MX_REG_FAH.databit.Rst_dsp=0;//Voice DSP Reset Control 0b: Normal 1b: Reset
	MX_REG_FAH.databit.Sel_pdm2_pi2c=1;//PDM2 PI2C Input Pin Select Pad_PDM2_DAT2
	MX_REG_FAH.databit.Sel_pdm1_pi2c=1;//PDM1 PI2C Input Pin Select Pad_PDM1_DAT1
	rt5670WriteReg(0xFA, MX_REG_FAH.data16);

	MX_REG_FBH.databit.En_cbj_tie_gr=0;//Combo Jack IN1N Tie Ground Control 0b: Disable
	MX_REG_FBH.databit.En_cbj_tie_gl=0;//Combo Jack IN1P Tie Ground Control 0b: Disable
	MX_REG_FBH.databit.En_pow_pdm1_ctrl=0;//Enable Control with PDM1 for power saving 0b: Disable
	MX_REG_FBH.databit.Sel_asrcin_clkout=0;//Auto Switch to RC Clock When ASRC Enable and MCLK isRemove,0 b: ref off, clock off,1b: ref off, clock keep
	MX_REG_FBH.databit.MD_PD=1;//Select New Depop Mode
	MX_REG_FBH.databit.Digital_gate_pad_ldo=1;//Enable gate mode with Pad Control and LDO
	MX_REG_FBH.databit.Sel_alc_zero_hp_time=0;//Select Auto turn off HP amp delay time 00'b: 0.25s
	MX_REG_FBH.databit.En_sel_in_r=1;//Enable Rch input path selection
	MX_REG_FBH.databit.En_sel_in_l=1;//Enable Lch input path selection
	MX_REG_FBH.databit.Sel_pow_capless=1;//Capless Power Gating Contr 1 Register control directly?pow_capless
	MX_REG_FBH.databit.Sel_cbj_mode=0;//Select Combo Jack Mode 0'b: Normal Mode(Combo Jack MIC L/R Controlled ByCBJ)
	MX_REG_FBH.databit.Sel_alc_zero_hp_mode=0;//Select Auto turn off HP amp Mode 0'b: Auto turn off HP amp 1'b: Auto turn off HP amp with Un-mute/Mute Depop
	MX_REG_FBH.databit.Sel_gpio7_out=0;//GPIO7 Pin Function Select 0b: GPIO7
	MX_REG_FBH.databit.Sel_adcdat1=0;//ADCDAT1 Pin Configuration 0b: Output
	MX_REG_FBH.databit.En_alc_zero_pow_hp=0;//Auto turn off HP amp control 0'b: Register control
	rt5670WriteReg(0xFB, MX_REG_FBH.data16);

	MX_REG_FCH.databit.temp_dac1_bias_ctrl=0;//Default Register Settings for HP Low Power Mode
	MX_REG_FCH.databit.Sel_pow_micbias2=1;//Pow_micbias2 Gating Control,1'b: Register control directly?pow_micbias2_digital
	MX_REG_FCH.databit.Auto_turnoff_micbias=0;//Auto turn off micbias1 and micbias2 control 0'b: Auto 1'b: Manual
	MX_REG_FCH.databit.Sel_reset_cbj=0;//Sel reset combo jack source,0'b: Reset by jd_combo_jack falling edge
	MX_REG_FCH.databit.Manual_reset_cbj=1;//Reset combo jack by manual control,0'b: Off (Reset to default)
	MX_REG_FCH.databit.temp_psv_dac=0;//Default Register Settings for HP Low Power Mode=>PR3D[8]
	MX_REG_FCH.databit.Sel_i2s3_data_mode=0;//Select I2S3 Data Mode,0b: Normal Mode1b: MONO Mode
	MX_REG_FCH.databit.Sel_pow_jd_debounce_source2=0;//Select IRQ power saving mode for JD Source2
	MX_REG_FCH.databit.Sel_irq_type=0;//Select type of the IRQ
	MX_REG_FCH.databit.Sel_tdm_data_mode=0;//Select TMD Data Mode,0b: Normal TDM Mode
	MX_REG_FCH.databit.Sel_i2s2_data_mode=0;//Select I2S2 Data Mode,0b: Normal Mode
	MX_REG_FCH.databit.Sel_tdm_rx_data=0;//Select TMD RX Data(Must Enable with Reg77[9])0b: ADC1L/ADC1R/ADC2L/ADC2R
	MX_REG_FCH.databit.Sel_pow_jd_debounce_source1=0;//Select IRQ power saving mode for JD Source1
	rt5670WriteReg(0xFC, MX_REG_FCH.data16);


	res |= rt5670WriteReg(0xfa, 0xc019);
	res |= rt5670WriteReg(0xfb, 0x0730);
	res |= rt5670WriteReg(0xfc, 0x0084);

	//res |= rt5670WriteReg(0xcd, 0x0010);	// ADC from DSP
	MX_REG_CDH.databit.Sel_if1_adc1_data_in3=0;//
	MX_REG_CDH.databit.Sel_dacl2_VAD_in=1;// Select DAC2_L VAD Input Source 1-->TxDP_ADC_R
	MX_REG_CDH.databit.Sel_debug_Source=0;// 
	MX_REG_CDH.databit.Sel_if1_adc1_txdp=1;//Select I2S1 ADCDAT Output Source :TxDP_ADC
	MX_REG_CDH.databit.Dummy_CD=0;//
	MX_REG_CDH.databit.Sel_dacr1_dac4_in=2;//TxDC_DAC_R
	MX_REG_CDH.databit.Sel_dacl1_dac4_in=2;//TxDC_DAC_L
	rt5670WriteReg(0xCD, MX_REG_CDH.data16);



	//CLOCK CONFIG///////////////////////////////////////////
	//rt5670WriteReg(0x7f, 0x1100);	   DSP Clock Control

	MX_REG_7FH.databit.sel_dsp_track0=0;//Select the track clock source for DSP,0000b: clk_sys
	MX_REG_7FH.databit.reserved=0;//
	MX_REG_7FH.databit.sel_i2s_pre_div5=1;//I2S_Pre_Div5,001b:/ 2
	MX_REG_7FH.databit.reserved2=0;//
	MX_REG_7FH.databit.sel_i2s_pre_div4=1;//I2S_Pre_Div4001b:/ 2
	MX_REG_7FH.databit.sel_i2s_bclk_ms4=0;//Fourth Master Mode clock relative of BCLK and LRCK,0b: 16Bits (32FS),1b: 32Bits (64FS)
	rt5670WriteReg(0x7F, MX_REG_7FH.data16);		

	rt5670WriteReg(0x80, 0x0000);
	rt5670WriteReg(0x81, 0x0000);
	rt5670WriteReg(0x82, 0x0000);
	rt5670WriteReg(0x83, 0x0000);		
	rt5670WriteReg(0x84, 0x0000);
	rt5670WriteReg(0x85, 0x0000);
	rt5670WriteReg(0x8a, 0x0000);

	res |= rt5670WriteReg(0xfa, 0xe011);
	res |= rt5670WriteReg(0xfa, 0xc011);


	res |= rt5670WriteReg(0x91, 0x0E06);


	MX_REG_62H.databit.reserved=1;//=1???
	MX_REG_62H.databit.En_serial_if=0;//
	MX_REG_62H.databit.reserved2=0;//
	MX_REG_62H.databit.Pow_pdm2=0;//  
	MX_REG_62H.databit.Pow_pdm1=0;//
	MX_REG_62H.databit.pow_adc_stereo2_filter=1;//
	MX_REG_62H.databit.pow_dac_monor_filter=1;//
	MX_REG_62H.databit.pow_dac_monol_filter=1;//
	MX_REG_62H.databit.pow_dac_stereo1_filter=1;//
	MX_REG_62H.databit.Pow_i2s_dsp=1;//
	MX_REG_62H.databit.Pow_adc_monor_filter=1;//
	MX_REG_62H.databit.Pow_adc_monol_filter=1;//
	MX_REG_62H.databit.Pow_adc_stereo_filter=1;//
	rt5670WriteReg(0x62, MX_REG_62H.data16);


	res |= rt5670WriteReg(0x62, 0xDf01);

	//res |= rt5670WriteReg(0x2A, 0x4646);
	//res |= rt5670WriteReg(0x53, 0x3000);

	res |= rt5670WriteReg(0x8F, 0x3100);
	res |= rt5670WriteReg(0x8E, 0x8019);

	//res |= rt5670WriteReg(0x03, 0x0808);
	//res |= rt5670WriteReg(0x1B, 0x0033);


	/* stop all */
	//rt5670Stop();




	for(i=0; i <10000; ++i);
	////////////////////////////////////////////////////////////////////////////////////////////// 
	/////////////////dsp config///////////////////
	FM36Iint();
	//////////////////////////////////////////////
	for(i=0; i <10000; ++i);


}

void FM36Iint(void)
{
	int res = 0;
	/////////////////////////dsp config////////////////////////////////////////////////////////////////////
	//Select internal profiles by setting Para[0x22F8, profile_index] as following:	
	//The profile settings of 0x8001 and 0x8003 are two microphones profiles for hands-free. The
	//profile setting 0x8005 is 1-mic hands-free. Hands-free modes provide AEC to cancel
	//acoustics echo and NS to suppress stationary ambient noise; in addition, 2-mic hands-free
	//support beam-forming feature to further suppress non-stationary noise.
	//res |= rt5670_dsp_write(0x22f8, 0x8003);//Internal profile 0x8003 //2-mic mode for hands-free, 16KHz sample rate
	res |= rt5670_dsp_write(0x22f8, 0x8005);//Internal profile 0x8005 //1-mic mode for hands-free, 16KHz sample rate
    //res |= rt5670_dsp_write(0x230c, 0x200);//Internal profile 0x8005 //1-mic mode for hands-free, 16KHz sample rate
    //res |= rt5670_dsp_write(0x230d, 0x100);//Internal profile 0x8005 //1-mic mode for hands-free, 16KHz sample rate
    
	res |= rt5670_dsp_write(0x22dd, 0);//
	res |= rt5670_dsp_write(0x22de, 0);//
	res |= rt5670_dsp_write(0x22e0, 0);//

	//Bit[3:0]: sampling rate.
	//=0x2: 16K normal mode, kernel process rate based on 16k
	res |= rt5670_dsp_write(0x2301, 0x0002);	// 0x0002 for 16k 2mic hands free
	//Bit[1:0]: number of microphones.0x0002 for 16k 2mic hands free
	//res |= rt5670_dsp_write(0x2302, 0x0002);	// 0x0002 for 16k 2mic hands free
	res |= rt5670_dsp_write(0x2302, 0x0001);	// 0x0002 for 16k 2mic hands free
	//Debug indicator. It shows how many frames have passed
	//res |= rt5670_dsp_write(0x2306, 0x063f);	

	DspTuneParaSetNS(); 
	DspTuneParaSetAEC();

	DspSetPath();	


	//res |= rt5670_dsp_write(0x22fc, 0x8000);	
	DSP_REG_22FCH.databit.bypass_pitch_prepare_aecref=0;
	DSP_REG_22FCH.databit.bypass_ne_pitch=0;//bypass ne pitch judge correction using far end pitch. 
	DSP_REG_22FCH.databit.bypass_pitch_prepare_LINEIN=0;
	DSP_REG_22FCH.databit.bypass_time_domain_AEC=0;
	DSP_REG_22FCH.databit.Revert_MIC0MIC1_LAEC_output=0;
	DSP_REG_22FCH.databit.bypass_DOA_search_ARSP_EST_proc=0;
	DSP_REG_22FCH.databit.reserved=0;//
	DSP_REG_22FCH.databit.Linein_direct_out=0; 
	DSP_REG_22FCH.databit.downlink_bypass=1;
	DSP_REG_22FCH.databit.bypass_all_signal_processing=0;
	rt5670_dsp_write(0x22fc, DSP_REG_22FCH.data16);//debug_flag 0x0000  

	DSP_REG_2303H.databit.reserved=0;
	DSP_REG_2303H.databit.Stress_Ratio_SPK_DRC=0;//bypass ne pitch judge correction using far end pitch. 
	DSP_REG_2303H.databit.common_SPK_DRC=0;
	DSP_REG_2303H.databit.reserved2=0;
	DSP_REG_2303H.databit.SPK_HARMONIC_GENERATION=0;
	DSP_REG_2303H.databit.reserved3=0;
	DSP_REG_2303H.databit.frequency_domain_AEC=1; 
	DSP_REG_2303H.databit.post_filter=1;
	DSP_REG_2303H.databit.beam_forming=0;
	DSP_REG_2303H.databit.echo_ase_proc_under_BF=0;
	DSP_REG_2303H.databit.reserved4=0;
	DSP_REG_2303H.databit.bypass_frequency_domain_process=0;
	rt5670_dsp_write(0x2303, DSP_REG_2303H.data16);//debug_flag 0x0710	
	//rt5670_dsp_write(0x2303, 0x0710);//debug_flag 0x0710	

	////
	DSP_REG_2304H.databit.adjustable_Line_out_HPF=0;
	DSP_REG_2304H.databit.Mic_in_HPF_Enable=0;//bypass ne pitch judge correction using far end pitch. 
	DSP_REG_2304H.databit.mic_pre_emphasis_filter=0;
	DSP_REG_2304H.databit.mic_de_emphasis_filter=0;
	DSP_REG_2304H.databit.Line_outHPF_filter=0;
	DSP_REG_2304H.databit.Line_out_HPF_Enable=0;
	DSP_REG_2304H.databit.LINEIN_EMPHFILTER=0;//
	DSP_REG_2304H.databit.LINEIN_DEEMPHFILTER=0; 
	DSP_REG_2304H.databit.Spk_out_HPF_Enable=0;
	DSP_REG_2304H.databit.Line_in_HPF_Enable=0;
	DSP_REG_2304H.databit.reserved=0;
	DSP_REG_2304H.databit.FENS_FFTONLY=0;
	DSP_REG_2304H.databit.FFP=1;
	rt5670_dsp_write(0x2304, DSP_REG_2304H.data16);//sp_flag 0x0323
	
  //res |= rt5670_dsp_write(0x2304, 0x0332);
	
	DSP_REG_2305H.databit.reserved=0;
	DSP_REG_2305H.databit.BVE=0;//
	DSP_REG_2305H.databit.reserved2=0;
	DSP_REG_2305H.databit.BAND_WIDTH_EXT1=0;
	DSP_REG_2305H.databit.BAND_WIDTH_EXT2=0;
	DSP_REG_2305H.databit.SHORT_DELAY_WIDE_BAND=0;
	DSP_REG_2305H.databit.Side_tone_Generation=0;
	DSP_REG_2305H.databit.reserved3=0;
	DSP_REG_2305H.databit.AVC=0;//
	rt5670_dsp_write(0x2305, DSP_REG_2305H.data16);//ft_flag 0x0000 for HF mode

	res |= rt5670_dsp_write(0x3fb5, 0x0000);///????	
	res |= rt5670_dsp_write(0x22fb, 0x0000);//DSP to start running
	/////////////////////////////////////////////////////////////////////////////////////////////////////		
}

void DspTuneParaSetNS(void)

{
    int res = 0;

    //forte aec_v4___ns-20171122
	//res |= rt5670_dsp_write(0x22f8, 0x8003);

	//he converging time of stationary NS is set by fqpara_period.Need to tune snesu_thr_sn_est(0x2386) together.
	res |= rt5670_dsp_write(0x2370,0x0005);  //1_8	8-->2S		
	
	//two mic parameters setting //  /////////////////////new 20171122
	res |= rt5670_dsp_write(0x2371, 0x0004);//?MIC??UNIT:cm
  
	///*
	//DOA RANGE
	res |= rt5670_dsp_write(0x2373, (10*32767)/180);//0,voice pickup angle1 00--->0,2000-->45
	res |= rt5670_dsp_write(0x2374, (170*32767)/180);//180,voice pickup angle2,7FFF-->180,6000->135
	//Acceptance Beam,margin to the accuracy of DOA estimate
	res |= rt5670_dsp_write(0x2379, (20*32767)/180);//20,level of ns: higer value means less suppression
	//Rejection Beam,rejection range of DOA
	res |= rt5670_dsp_write(0x237a, (30*32767)/180);//30,reject level
	//This parameter will allow an immediate talking direction switch when DOA makes a suddenand wide-angle change
	res |= rt5670_dsp_write(0x23a2, 0x2AAB);//0x2AAB//BF reset threshold in HF mode. Range: 0~0x7FFF
	///////////////////////////////////////////////////////
	//*/

	//Harmonic Boosting.This function will adjust the final VAD judgment based on observation of VADs at all
	//harmonic bins. It is particular useful in a reverberating environment where the detection of in-beam speech is difficult.
	//res |= rt5670_dsp_write(0x237e, 0x0000);//set to 1 will enable this function.

	res |= rt5670_dsp_write(0x237e, 0x0000);//0x0001//set to 1 will enable this function.

	//Over boosting if hmnc_bst_thr<0x4000; no boosting if hmnc_bst_thr==0x7FFF. Smaller value leads more boosting.
	//res |= rt5670_dsp_write(0x237f, 0x6000);//set smaller value can keep more NE voice, but may do less suppression on voice-like noise, such as babble noise.
	res |= rt5670_dsp_write(0x237f, 0x7FFF);

	//Set snesu_thr_sn_est to smaller value will keep more NE voice under stationary noise, but it might results in watering sounded noise.
	res |= rt5670_dsp_write(0x2386, 0x300);  //1_0x0300   2_1000
	//higher value means more staionary noise in high frequency will be suppression */
	res |= rt5670_dsp_write(0x2387, 0x300);	//1_0x0300	 2_1000	
	
	//Compensation factor on stationary noise power estimation. Range in 0 ~
	//0x7FFF, Q15 format. Smaller value leads to less stationary noise
	//suppression and speech loss.
	res |= rt5670_dsp_write(0x2388, 0x7fff);

	//b_post_flt is a compensation factor multiplied to non-stationary noise power estimated. Set
	//it to smaller value means less non-stationary noise suppression and speech lose.
	//small value means less non-stationary noise suppression (0-7fff)
	//res |= rt5670_dsp_write(0x2389, 0x2000);//1_0x7fff(ok)  
	res |= rt5670_dsp_write(0x2389, 0x7fff);//1_0x7fff(ok)
	
	//Compensation factor on residual echo power estimation, Range in 0 ~
	//0x7FFF, 0x2000 is unity gain. Smaller value leads to less residual echo suppression and speech loss.
	//Compensation factor on residual echo power estimation
	res |= rt5670_dsp_write(0x238a, 0x4000);	
	//Maximum NS Level,The signal processing algorithm will adjust the targeting NS level within this range depending on the signal and noise conditions.
	//less value means more noise suppression 
	//18db
	res |= rt5670_dsp_write(0x238b, 0x3000);//min_g_ctrl_maxg,Upper bound of noise suppression level in frequency domain, 0x7fff is 0dB.238b_238c_23a1
	res |= rt5670_dsp_write(0x238c, 0x3000);//min_g_ctrl_ming,Lower bound of noise suppression level,Set min_g_ctrl_ming <= min_g_ctrl_maxg


	//Maximum NS Level,The signal processing algorithm will adjust the targeting NS level within this range depending on the signal and noise conditions.
	//less value means more noise suppression */
	//18db
	//res |= rt5670_dsp_write(0x238b, 0x1000*2);//min_g_ctrl_maxg,Upper bound of noise suppression level in frequency domain, 0x7fff is 0dB. 238b_238c_23a1
	//24db
	//res |= rt5670_dsp_write(0x238c, 0x0800*2);//min_g_ctrl_ming,Lower bound of noise suppression level,Set min_g_ctrl_ming <= min_g_ctrl_maxg	

	//far end ////////////////////////////////////////////////////////////////////////////
	//Minimum noise suppression gain for each bin for far end. Range:0x0000~0x7FFF.
	//small value means less stationary noise suppression (0-7fff)
	//res |= rt5670_dsp_write(0x23C4, 0x1000); //1_new	
	//Threshold for stationary noise decision for far end. Range in [0x8000,0x7FFF], Q10 format. Smaller value leads to under noise estimate.
	res |= rt5670_dsp_write(0x23C7, 0x1000);  //1_new	   2_1000			
	res |= rt5670_dsp_write(0x23C4, 0x0400); //1_new
	
	
	//res |= rt5670_dsp_write(0x2398, 0x4668);
	//Multiple bands Line-out EQ, 4-bits for one band, default 0x4444 means
	//0dB for all 4 bands.
	//EQ gain setting (4-bits):
	//0: +6dB, 1: +4dB, 2: +2dB, 3: +1dB,
	//4: 0dB, 5: -1dB, 6: -2dB, 7: -4dB,
	//8: -6dB, 9: -8dB, A: -10dB, B: -13dB,
	//C: -16dB, D: -20dB, E: -30dB, F: -50dB.		
	//res |= rt5670_dsp_write(0x2398, 0x4444);
	//res |= rt5670_dsp_write(0x2398, 0x4668);
	//res |= rt5670_dsp_write(0x2397, 0x4456);//Bit[15~12]: 4583.7 ~ 4958.73, Bit[11~8]: 4958.73 ~ 5333.76Hz,Bit[7~4]: 5333.76 ~ 5708.79Hz, Bit[3~0]: 5708.79 ~ 6083.82 Hz;
	//res |= rt5670_dsp_write(0x2398, 0x99bb);//Bit[15~12]: 6083.82 ~ 6458.85Hz, Bit[11~8]: 6458.85 ~ 6833.88Hz,Bit[7~4]: 6833.88 ~ 7208.91Hz, Bit[3~0]: 7208.91 ~ 7583.94 Hz;	

	///*
	//pitch
	res |= rt5670_dsp_write(0x2375, 0x7ff0);
	res |= rt5670_dsp_write(0x2376, 0x7990);
	res |= rt5670_dsp_write(0x2377, 0x7332);	
	//ffp	
	res |= rt5670_dsp_write(0x23a8, 0x471c);
	res |= rt5670_dsp_write(0x23a9, 0x471c);
	res |= rt5670_dsp_write(0x23aa, 0x005A);
	res |= rt5670_dsp_write(0x23ab, 0x7c00);	

	//*/

    ///////////////////////////////////////////////////////////////////////////////////////

	//b_post_flt is a compensation factor multiplied to non-stationary noise power estimated. Set
	//it to smaller value means less non-stationary noise suppression and speech lose.
	//small value means less non-stationary noise suppression (0-7fff)
	res |= rt5670_dsp_write(0x2389, 0x2000);//1_0x7fff(ok)  

	//In the presence of non-stationary noise, additional noise suppression can be applied when
	//the noise residue sounds annoying in the high-frequency range. These annoying residues
	//typically appear as discontinuous pepper-like interferences in the spectrogram. To suppress
	//them, the value of k_pepper can be set corresponding to the starting frequency above
	//which more suppression needs to be applied.
	//peper-like noise

	//res |= rt5670_dsp_write(0x239D, 0x000C ); //(0-180),The default frequency point is 500Hz: 16k mode, bin resolution is 41.67Hz, then k_pepper = 500/41.67 = 0xC.

	//Bigger value will give less suppression on pepper. Range in [0x0400,0x7fff], Q10 format. Unity gain: 0x400

	//res |= rt5670_dsp_write(0x239E, 0x0400); //The smaller is a_pepper, the more noise suppression will be applied. A too small value of a_pepper may result in speech attenuated.  

	//This parameter provides additional control on the NS level for stationary noise.
	//As for the final NS level (for stationary noise), it is decided by the bigger value setting in sn_c_f or min_g_ctrl_ming. 
	//0x800-->NS level is 24dB.0x4000-->NS level is 6dB
	//res |= rt5670_dsp_write(0x23a1, 0x400);  //The default value of sn_c_f is 0, which means final NS level for stationary noise is only decided by min_g_ctrl_maxg and min_g_ctrl_ming.	
	res |= rt5670_dsp_write(0x23a1, 0x0000);  //The default value of sn_c_f is 0, which means final NS level for stationary noise is only decided by min_g_ctrl_maxg and min_g_ctrl_ming.	



	//Noise Paste gain in Q14 format. Range: 0x4000 ~ 0x7FFF (1~2)
	res |= rt5670_dsp_write(0x23a3, 0x4000);	  

	//Compensation factor on stationary noise power estimation. Range in 0 ~
	//0x7FFF, Q15 format. Smaller value leads to less stationary noise suppression and speech loss.
	res |= rt5670_dsp_write(0x2388, 0x7fff);

	//This function will provide extra noise suppression on each bin based on observation of its neighboring speech activities.

	//res |= rt5670_dsp_write(0x239b, 0x0001);//extra noise suppression control, valid value is 0~2. Set it to 0 to disable extra noise suppression, set higher value to suppress more.

	//extra noise suppression: small value means more extra noise suppression(400-7fff)

	//res |= rt5670_dsp_write(0x239c, 0x0800);//extra noise suppression gain. Set it to smallervalue to suppress more. Its range is from 0x400~0x7FFF.


	///////////////////////////////////////////////////////////////////////////

		
	res |= rt5670_dsp_write(0x23a3, 0x4000);//Noise Paste gain in Q14 format. Range: 0x4000 ~ 0x7FFF (1~2)
		
	//res |= rt5670_dsp_write(0x230c, 0x1000); 
	//res |= rt5670_dsp_write(0x230d, 0x1000); 		
	//res |= rt5670_dsp_write(0x230c, 0x0200);//Microphone volume. Range: 0~0x7fff. Unity gain: 0x100	
	//res |= rt5670_dsp_write(0x230d, 0x0080);//Speaker volume. Range: 0~0x7fff. Unity gain: 0x100
	//res |= rt5670_dsp_write(0x2310, 0x0010);//Spkout 500Hz HPF

}



void DspSetPath(void)
{
	int res = 0;	

	// mic_pga 	
	//Serial port 0 format. Effective in init stage only.	
	res |= rt5670_dsp_write(0x2260, 0x30d9);//0x78D9
	//Serial port 1 format.Definition refers to SP0_control.
	res |= rt5670_dsp_write(0x2261, 0x30d9);// 
	
	//DIV_1: clock divider. Range in 0x5 ~ 0x7F. Effective in init stage only
	res |= rt5670_dsp_write(0x226c, 0x000c);	//
	//MUL_0: clock multiplie. Range in 0x5 ~ 0xFF or 0 (0 means 256). Effective in init stage only.
	res |= rt5670_dsp_write(0x226d, 0x000c);	
	//MCLK system clock setting. Effective in init stage only.	
	res |= rt5670_dsp_write(0x226e, 0x0022);//MCLK=4.096M		
	//MIC01 PGA gain for PDM in. D15~8: MIC1 volume setting; D7~0: MIC0 volume
	//setting. 0xE0,0xF0, 0xFC, 0x0, 0x10, 0x14.+24dB, +12dB, +3dB, 0dB, -12dB, -15dB. Range:-	
  res |= rt5670_dsp_write(0x2278, 0x0000);//0XE0E0 24DB	

	//0x2289~0x2294
	res |= rt5670_dsp_write(0x2289, 0x7fff);//mic0 0db	
	res |= rt5670_dsp_write(0x2290, 0x7fff);//mic1 0db
	res |= rt5670_dsp_write(0x2291, 0);//0 means mute
	res |= rt5670_dsp_write(0x2292, 0);//0 means mute
	res |= rt5670_dsp_write(0x2293, 0);//0 means mute
	res |= rt5670_dsp_write(0x2294, 0);//0 means mute
	//Line-in mixer gain, only effective during init.Unity gain: 0x7FFF.
	res |= rt5670_dsp_write(0x2298, 0x7fff);//lin_l 0db	
	res |= rt5670_dsp_write(0x2299, 0x7fff);//lin_r 0db
	res |= rt5670_dsp_write(0x229D, 0);//0 means mute
	res |= rt5670_dsp_write(0x229E, 0);//0 means mute	
	
	res |= rt5670_dsp_write(0x22b5, 0);//0 means mute
	res |= rt5670_dsp_write(0x22b6, 0);//0 means mute
	res |= rt5670_dsp_write(0x22b7, 0);//0 means mute
	res |= rt5670_dsp_write(0x22b8, 0);//0 means mute		
	
	//MIC source select, set this parameter according to real clock source connection0: PDM, 1: SP0, 2: SP1, 3: SP2
	res |= rt5670_dsp_write(0x2288, 0x0002);	
	res |= rt5670_dsp_write(0x2282, 0x0008);	//MIC0-->SPI1_SLOT0
	res |= rt5670_dsp_write(0x2283, 0x0009);	//MIC1-->SPI1_SLOT1	
	
	//Line-in source select, set this parameter according to real clock source connection0: PDM, 1: SP0, 2: SP1, 3: SP2
	res |= rt5670_dsp_write(0x2295, 0x0001);
    res |= rt5670_dsp_write(0x2296, 0x0000);//LIN_L-->SPI0_SLOT0
	res |= rt5670_dsp_write(0x2297, 0x0001);//LIN_R-->SPI0_SLOT1
		
	//Lout destination select, set this parameter according to real clock source connection0: PDM, 1: SP0, 2: SP1, 3: SP2
	res |= rt5670_dsp_write(0x22b2, 0x0002);
	res |= rt5670_dsp_write(0x22d7, 0x0008);	// LINEOUT_L-->SPI1_SLOT0
	res |= rt5670_dsp_write(0x22d8, 0x0009);	// LINEOUT_R-->SPI1_SLOT1

	//Spkout destination select, set this parameter according to real clock source connection 0: PDM, 1: SP0, 2: SP1, 3: SP2
	res |= rt5670_dsp_write(0x22b3, 0x0001);
	res |= rt5670_dsp_write(0x22d9, 0x0000);	// SPEAK_OUT_L-->SPI0_SLOT0
	res |= rt5670_dsp_write(0x22da, 0x0001);	// SPEAK_OUT_R-->SPI0_SLOT1
	//0x1006 Last_lout_buf
	res |= rt5670_dsp_write(0x22c1, 0x1006);//Line_out L & R
	res |= rt5670_dsp_write(0x22c2, 0x1006);//Line_out L & R	
	//res |= rt5670_dsp_write(0x22c1, 0x1000);//Line_out L & R
	//res |= rt5670_dsp_write(0x22c2, 0x1001);//Line_out L & R		
	//0x1007 Last_spkout_buf
	res |= rt5670_dsp_write(0x22c3, 0x1007);//spkout L & R
	res |= rt5670_dsp_write(0x22c4, 0x1007);//spkout L & R	

    res |= rt5670_dsp_write(0x22EB, 0x0002);//Actual MIC number in system
	res |= rt5670_dsp_write(0x22EE, 0x0000);//Assign main interrupt process. Effective in init stage only.0x0 means SP0 as main processing.
	//Set maximal MIPs. Effective in init stage only.MIPs = 1.024 * (1 + Bit[6~0] of 0x22f2)
	res |= rt5670_dsp_write(0x22F2, 0x004c);
	res |= rt5670_dsp_write(0x22FA, 0x003F);//D0: =1, mic0 enable D1: =1, mic1 enable D2: =1, mic2 enable D3: =1, mic3 enable D4: =1, mic4 enable D5: =1, mic5 enable
	
	//Interrupt mask.
	res |= rt5670_dsp_write(0x22fd, 0x001e);//SP0 interrupt, 0x08: SP1 interrupt	

}

void DspTuneParaSetAEC(void)
{
	int res = 0;	
	int i=0;

	res |= rt5670_dsp_write(0x232b, 0x0090);//MIC delay, 9ms for 16kHz 
	res |= rt5670_dsp_write(0x232f, 0x00d0+0x50);//Gain for AEC reference channel. Unity gain: 0x100. Q: up or down
	
	/* Minimal EQ value for linear residual echo estimate on 13 sub-bands. Range: 0~0x7FFF */
	/*
	res |= rt5670_dsp_write(0x2355, 0x0888);
	res |= rt5670_dsp_write(0x2356, 0x1888);
	res |= rt5670_dsp_write(0x2357, 0x2888);
	res |= rt5670_dsp_write(0x2358, 0x3888);
	res |= rt5670_dsp_write(0x2359, 0x4888);
	res |= rt5670_dsp_write(0x235a, 0x5888);
	res |= rt5670_dsp_write(0x235b, 0x6888);
	res |= rt5670_dsp_write(0x235c, 0x7000);
	res |= rt5670_dsp_write(0x235d, 0x7888);
	res |= rt5670_dsp_write(0x235e, 0x7fff);
	res |= rt5670_dsp_write(0x235f, 0x7fff);
	res |= rt5670_dsp_write(0x2360, 0x7fff);
	res |= rt5670_dsp_write(0x2361, 0x7fff);
	*/

	
	res |= rt5670_dsp_write(0x2362, 0x1000);//Threshold for residual echo estimate. The smaller value it is, the less amount of echo estimated in. Range: 0~0x7FFF
	res |= rt5670_dsp_write(0x2367, 0x0007);//Number of holding on frames when single talk is detected(1-10)
	res |= rt5670_dsp_write(0x2368, 0x4000);//Echo active detection threshold. The smaller value, the harder to detect echo
	res |= rt5670_dsp_write(0x2369, 0x0008);//Double talk holding time, unit is frame. Range: 1~0x10 
	res |= rt5670_dsp_write(0x236a, 0x6500);//Factor for forward cross-bin nonlinear echo estimation. Range: 0~0x7FFF
	res |= rt5670_dsp_write(0x236b, 0x0009);//This parameter specifies the cutoff frequency of the loudspeaker
	res |= rt5670_dsp_write(0x236c, 0x003c);//low bound of high frequency and assign it to DT_Cut_K
	res |= rt5670_dsp_write(0x236d, 0x0000);//Threshold to apply additional echo suppression for high frequency bands. Smaller value means heavier suppression. Range: 0~0x7FFF. Set it to 0 will disable this function
	res |= rt5670_dsp_write(0x236f, 0x6500);//Factor for backward cross-bin nonlinear echo estimation. Range: 0~0x7FFF
	
	res |= rt5670_dsp_write(0x23ad, 0x2000);//Threshold 1 of double talk detection
	res |= rt5670_dsp_write(0x23ae, 0x2000);
	res |= rt5670_dsp_write(0x23af, 0x2000);
		
	res |= rt5670_dsp_write(0x23b4, 0x2000);//Threshold 2 of double talk detection 
	res |= rt5670_dsp_write(0x23b5, 0x2000);
	res |= rt5670_dsp_write(0x23b6, 0x2000);
	
	res |= rt5670_dsp_write(0x23bb, 0x6000);//Threshold 3 of double talk detection	
		
	/* main parameter to sup echo; higer value maens more sup(0-7fff) */
	 //res |= rt5670_dsp_write(0x238a, 0x0400);//A bigger value results in more suppression on echo, but may attenuate the near-end speech as well
	//res |= rt5670_dsp_write(0x238a, 0x7f00);
	
	
}



void rt5670ReadAll()
{
	uint16_t data = 0, reg;
	uint16_t aec_addr[] = {
		0x2260, 0x2261, 0x229d, 0x232b,
		0x22f8, 0x22fa, 0x22fc, 0x232f,
		0x2355, 0x2356, 0x2357, 0x2358,
		0x2359, 0x235a, 0x235b, 0x235c,
		0x235d, 0x235e, 0x235f, 0x2360,
		0x2361, 0x2362, 0x2367, 0x2368,
		0x2369, 0x236a, 0x236b, 0x236c,
		0x236d, 0x236f, 0x2370, 0x2371,
		0x2373, 0x2374, 0x2375, 0x2376,
		0x2377, 0x2379, 0x237a, 0x2388,
		0x2389, 0x238a, 0x238b, 0x238c,
		0x2290, 0x2288, 0x22b2, 0x2295,
		0x22b3, 0x22d9, 0x22da, 0x22fd,
		0x22c1, 0x22c2, 0x22c3, 0x22c4,
		0x229d, 0x232b, 0x232f, 0x23c4,
		0x2398, 0x23a1, 0x23a3, 0x23ad,
		0x23ae, 0x23af, 0x23b4, 0x23b5,
		0x23b6, 0x23bb, 0x2303, 0x2304,
		0x230c, 0x230d, 0x2310, 0x22fb};

	uint16_t rt5670_dsp_data_src[] = {
		0x2261, 0x2282, 0x2283, 0x22d7, 0x22d8};
	uint16_t rt5670_dsp_rate_par[] = {
		0x226c, 0x226d, 0x226e, 0x22f2, 0x2301, 0x2306};

    for (int i = 0; i < 0xff; i++) {
        rt5670ReadReg(i, &data);
        printf("%02X: %04X\n", i, data);
    }

	printf("Index registers:\n");
	for (int i = 0; i < 0x44; i++) {
        rt5670_index_read(i, &data);
        printf("%02X: %04X\n", i, data);
    }

	printf("DSP registers:\n");
	for (int i = 0; i < sizeof(aec_addr)/sizeof(uint16_t); i++) {
        rt5670_dsp_read(aec_addr[i], &data);
        printf("%04X: %04X\n", aec_addr[i], data);
    }
	for (int i = 0; i < sizeof(rt5670_dsp_data_src)/sizeof(uint16_t); i++) {
        rt5670_dsp_read(rt5670_dsp_data_src[i], &data);
        printf("%04X: %04X\n", rt5670_dsp_data_src[i], data);
    }
	for (int i = 0; i < sizeof(rt5670_dsp_rate_par)/sizeof(uint16_t); i++) {
        rt5670_dsp_read(rt5670_dsp_rate_par[i], &data);
        printf("%04X: %04X\n", rt5670_dsp_rate_par[i], data);
    }
}

int rt5670Init(Rt5670Config *cfg)
{
	int res = 0;

	res = I2sInit(); // ESP32 in master mode
	res |= rt5670WriteReg(0x00, 0x0000);
	res |= rt5670WriteReg(0x0a, 0x6021);
	res |= rt5670WriteReg(0x0b, 0x20A7);
	res |= rt5670WriteReg(0x0d, 0x0008);
	res |= rt5670WriteReg(0x0e, 0x3000);
	//res |= rt5670WriteReg(0x1d, 0xafaf);
	res |= rt5670WriteReg(0x26, 0x3c20);
	res |= rt5670WriteReg(0x27, 0x3820);
	res |= rt5670WriteReg(0x28, 0x3c71); /* ADC3-MONO_L */
	res |= rt5670WriteReg(0x2d, 0x6000);/////////////////////////////////////////////////////////////////jcp change 20170920
	//res |= rt5670WriteReg(0x2d, 0xa000);
	if (cfg->adcInput & ADC_INPUT_IN1P)
		res |= rt5670WriteReg(0x3c, 0x007D); /* BST1 */
	if (cfg->adcInput & ADC_INPUT_IN3P)
		res |= rt5670WriteReg(0x3e, 0x0077);
	if (cfg->adcInput & ADC_INPUT_IN2P)
		res |= rt5670WriteReg(0x40, 0x001b); /* BST2-ADC3 */
	res |= rt5670WriteReg(0x61, 0x980E);
	res |= rt5670WriteReg(0x62, 0xD001);
	res |= rt5670WriteReg(0x63, 0xf8fd);
	wait_1us(10000);
	res |= rt5670WriteReg(0x64, 0xEC74); /* BST1 power */
	res |= rt5670WriteReg(0x65, 0x0E01);
	res |= rt5670WriteReg(0x6a, 0x003D);
	res |= rt5670WriteReg(0x6c, 0x3640);
	res |= rt5670WriteReg(0x6a, 0x0039);
	res |= rt5670WriteReg(0x6c, 0x3500);
	res |= rt5670WriteReg(0x73, 0x2220);
	//res |= rt5670WriteReg(0x77, 0x0e00); //////////////////////////////////////////////////////////////jcp add 20170920
	res |= rt5670WriteReg(0xfa, 0xc019);
	res |= rt5670WriteReg(0xfb, 0x0730);
	res |= rt5670WriteReg(0xfc, 0x0084);

	res |= rt5670WriteReg(0xcd, 0x0010);	// ADC from DSP

	res |= rt5670WriteReg(0x7f, 0x1100);	/* disable DSP tracking */
	res |= rt5670WriteReg(0x80, 0x0000);
	res |= rt5670WriteReg(0x81, 0x0000);
	res |= rt5670WriteReg(0x82, 0x0000);
	res |= rt5670WriteReg(0x83, 0x0000);	/* disable DSP tracking */
	res |= rt5670WriteReg(0x84, 0x0000);
	res |= rt5670WriteReg(0x85, 0x0000);
	res |= rt5670WriteReg(0x8a, 0x0000);

	res |= rt5670WriteReg(0xfa, 0xe011);
	res |= rt5670WriteReg(0xfa, 0xc011);


#if 1  //forte aec_v4___ns-20171122
	res |= rt5670_dsp_write(0x22f8, 0x8003);

	res |= rt5670_dsp_write(0x2370, 0x0005);  //1_8
	
	/* two mic parameters setting */  /////////////////////new 20171122
	res |= rt5670_dsp_write(0x2371, 0x0006);
	res |= rt5670_dsp_write(0x2373, 0x0000);//voice pickup angle1
	res |= rt5670_dsp_write(0x2374, 0x7fff);//voice pickup angle2
	res |= rt5670_dsp_write(0x2379, 0x7500);//level of ns: higer value means less suppression
	res |= rt5670_dsp_write(0x237a, 0x7600);//reject level
	res |= rt5670_dsp_write(0x23a2, 0x1000);//switch level

	/* smaller value, high natural voice */
	res |= rt5670_dsp_write(0x2386, 0x1000);  //1_0x0300   2_1000

	/* higher value means more staionary noise in high frequency will be suppression */
	res |= rt5670_dsp_write(0x2387, 0x1000);  //1_0x0300   2_1000
	res |= rt5670_dsp_write(0x23C7, 0x1000);  //1_new	   2_1000

	/* small value means less stationary noise suppression (0-7fff) */
	res |= rt5670_dsp_write(0x2388, 0x7fff);

	/* small value means less non-stationary noise suppression (0-7fff) */
	res |= rt5670_dsp_write(0x2389, 0x7fff);//1_0x7fff(ok)

	res |= rt5670_dsp_write(0x238a, 0x4000);

	/* less value means more noise suppression */
	res |= rt5670_dsp_write(0x238b, 0x0400); //238b_238c_23a1
	res |= rt5670_dsp_write(0x238c, 0x0400);

	
	res |= rt5670_dsp_write(0x23C4, 0x0400); //1_new
	
	res |= rt5670_dsp_write(0x2398, 0x4668);

	/* extra noise suppression: highter value means more extra noise suppression(0-2) */ 
	//res |= rt5670_dsp_write(0x239b, 0x0002);
	/* extra noise suppression: small value means more extra noise suppression(400-7fff) */ 
	//res |= rt5670_dsp_write(0x239c, 0x0400);


	/* peper-like noise */
	//res |= rt5670_dsp_write(0x239D, 0x0400); //(0-180)
	//res |= rt5670_dsp_write(0x239E, 0x0400); //biger value means less sp(400-7fff)


	/* same as 238b/c */
	res |= rt5670_dsp_write(0x23a1, 0x0400);  //1_0x2000


	
	res |= rt5670_dsp_write(0x23a3, 0x4000);//Noise Paste gain in Q14 format. Range: 0x4000 ~ 0x7FFF (1~2)
	

	//res |= rt5670_dsp_write(0x230c, 0x1000); 
	//res |= rt5670_dsp_write(0x230d, 0x1000); 

	
	res |= rt5670_dsp_write(0x230c, 0x0200);//Microphone volume. Range: 0~0x7fff. Unity gain: 0x100
	res |= rt5670_dsp_write(0x230d, 0x0080);//Speaker volume. Range: 0~0x7fff. Unity gain: 0x100
	res |= rt5670_dsp_write(0x2310, 0x0010);//Spkout 500Hz HPF

	/*---------------------Data path selection-------------------------*/
	res |= rt5670_dsp_write(0x22fc, 0x6000);
	res |= rt5670_dsp_write(0x229d, 0x0000);
	res |= rt5670_dsp_write(0x2260, 0x30d9);
	res |= rt5670_dsp_write(0x2260, 0x30d9);

	/* mic_pga */
	res |= rt5670_dsp_write(0x2278, 0xe0e0);
	//res |= rt5670_dsp_write(0x2278, 0xf0f0); //1_0xe0e0
	//res |= rt5670_dsp_write(0x2279, 0xf0f0);
	//res |= rt5670_dsp_write(0x227a, 0xf0f0);
	
	res |= rt5670_dsp_write(0x2289, 0x7fff);
	res |= rt5670_dsp_write(0x2290, 0x7fff);
	res |= rt5670_dsp_write(0x2288, 0x0002);
	res |= rt5670_dsp_write(0x22b2, 0x0002);
	res |= rt5670_dsp_write(0x2295, 0x0001);
	res |= rt5670_dsp_write(0x22b3, 0x0001);
	res |= rt5670_dsp_write(0x22da, 0x0001);
	res |= rt5670_dsp_write(0x22fd, 0x001e);
	res |= rt5670_dsp_write(0x22c1, 0x1006);
	res |= rt5670_dsp_write(0x22c2, 0x1006);
	res |= rt5670_dsp_write(0x22c3, 0x1007);
	res |= rt5670_dsp_write(0x22c4, 0x1007);
	res |= rt5670_dsp_write(0x2261, 0x30d9);	// from StereoADC
	res |= rt5670_dsp_write(0x2282, 0x0008);	// from StereoADC
	res |= rt5670_dsp_write(0x2283, 0x0009);	// from StereoADC
	res |= rt5670_dsp_write(0x22d7, 0x0008);	// from StereoADC
	res |= rt5670_dsp_write(0x22d8, 0x0009);	// from StereoADC
	res |= rt5670_dsp_write(0x226c, 0x000c);	// sysclk 12.288Mhz
	res |= rt5670_dsp_write(0x226d, 0x000c);	// sysclk 12.288Mhz
	res |= rt5670_dsp_write(0x226e, 0x0022);	// sysclk 12.288Mhz
	res |= rt5670_dsp_write(0x22f2, 0x004c);
	res |= rt5670_dsp_write(0x2301, 0x0002);	// rate 16Khz
	res |= rt5670_dsp_write(0x2306, 0x063f);
	res |= rt5670_dsp_write(0x3fb5, 0x0000);
	
	
	/*---------------------Function selection-------------------------*/
	res |= rt5670_dsp_write(0x2303, 0x0200); //1_230  2_0x0600
	res |= rt5670_dsp_write(0x2304, 0x0000);	
	res |= rt5670_dsp_write(0x22fb, 0x0000);
#endif





#if 1 //new DSP  AEC setting
		res |= rt5670_dsp_write(0x22f8, 0x8003);
	
		
#if 1 //AEC	
		res |= rt5670_dsp_write(0x232b, 0x0090);//MIC delay, 9ms for 16kHz 
		res |= rt5670_dsp_write(0x232f, 0x00d0);//Gain for AEC reference channel. Unity gain: 0x100. Q: up or down
		
		/* Minimal EQ value for linear residual echo estimate on 13 sub-bands. Range: 0~0x7FFF */	
		res |= rt5670_dsp_write(0x2355, 0x0888);
		res |= rt5670_dsp_write(0x2356, 0x1888);
		res |= rt5670_dsp_write(0x2357, 0x2888);
		res |= rt5670_dsp_write(0x2358, 0x3888);
		res |= rt5670_dsp_write(0x2359, 0x4888);
		res |= rt5670_dsp_write(0x235a, 0x5888);
		res |= rt5670_dsp_write(0x235b, 0x6888);
		res |= rt5670_dsp_write(0x235c, 0x7000);
		res |= rt5670_dsp_write(0x235d, 0x7888);
		res |= rt5670_dsp_write(0x235e, 0x7fff);
		res |= rt5670_dsp_write(0x235f, 0x7fff);
		res |= rt5670_dsp_write(0x2360, 0x7fff);
		res |= rt5670_dsp_write(0x2361, 0x7fff);
		
		
		res |= rt5670_dsp_write(0x2362, 0x1000);//Threshold for residual echo estimate. The smaller value it is, the less amount of echo estimated in. Range: 0~0x7FFF
		res |= rt5670_dsp_write(0x2367, 0x0007);//Number of holding on frames when single talk is detected(1-10)
		res |= rt5670_dsp_write(0x2368, 0x4000);//Echo active detection threshold. The smaller value, the harder to detect echo
		res |= rt5670_dsp_write(0x2369, 0x0008);//Double talk holding time, unit is frame. Range: 1~0x10 
		res |= rt5670_dsp_write(0x236a, 0x6500);//Factor for forward cross-bin nonlinear echo estimation. Range: 0~0x7FFF
		res |= rt5670_dsp_write(0x236b, 0x0009);//This parameter specifies the cutoff frequency of the loudspeaker
		res |= rt5670_dsp_write(0x236c, 0x003c);//low bound of high frequency and assign it to DT_Cut_K
		res |= rt5670_dsp_write(0x236d, 0x0000);//Threshold to apply additional echo suppression for high frequency bands. Smaller value means heavier suppression. Range: 0~0x7FFF. Set it to 0 will disable this function
		res |= rt5670_dsp_write(0x236f, 0x6500);//Factor for backward cross-bin nonlinear echo estimation. Range: 0~0x7FFF
	
		res |= rt5670_dsp_write(0x2375, 0x7ff0);//A threshold for pitch detection. Range in [0, 0x7FFF]. Smaller value leads to lower false alarm and probability of detection.
		res |= rt5670_dsp_write(0x2376, 0x7990);
		res |= rt5670_dsp_write(0x2377, 0x7332);
		
		res |= rt5670_dsp_write(0x23a8, 0x471c);//Far Field Pickup
		res |= rt5670_dsp_write(0x23a9, 0x471c);
		res |= rt5670_dsp_write(0x23aa, 0x005A);
		res |= rt5670_dsp_write(0x23ab, 0x7c00);//Smoothing factor:Smaller value means FFP function will apply gain/reduction faster
		
		
		res |= rt5670_dsp_write(0x23ad, 0x2000);//Threshold 1 of double talk detection
		res |= rt5670_dsp_write(0x23ae, 0x2000);
		res |= rt5670_dsp_write(0x23af, 0x2000);
		
		
		res |= rt5670_dsp_write(0x23b4, 0x2000);//Threshold 2 of double talk detection 
		res |= rt5670_dsp_write(0x23b5, 0x2000);
		res |= rt5670_dsp_write(0x23b6, 0x2000);
		
		res |= rt5670_dsp_write(0x23bb, 0x6000);//Threshold 3 of double talk detection	
		
		
		/* main parameter to sup echo; higer value maens more sup(0-7fff) */
		//1_0x4000
		res |= rt5670_dsp_write(0x238a, 0x7f00);//A bigger value results in more suppression on echo, but may attenuate the near-end speech as well
#endif //AEC_END	
		
		
#if 1 //NS	
		res |= rt5670_dsp_write(0x2370, 0x0008);  //1_8 
		
		res |= rt5670_dsp_write(0x2388, 0x7fff);//Smaller value leads to less stationary noise suppression and speech loss
		res |= rt5670_dsp_write(0x2389, 0x7000);//Set it to smaller value means less non-stationary noise suppression and speech lose
	
		//1_0x0400
		res |= rt5670_dsp_write(0x238b, 0x1000);//low bound of high frequency and assign it to DT_Cut_K
		res |= rt5670_dsp_write(0x238c, 0x1000);
		
		res |= rt5670_dsp_write(0x2398, 0x4668);//_fqpara_equal-------------unknow
		
		res |= rt5670_dsp_write(0x23a1, 0x1000);//provides additional control on the NS level for stationary noise
		
		res |= rt5670_dsp_write(0x23a3, 0x4000);//Noise Paste gain in Q14 format. Range: 0x4000 ~ 0x7FFF (1~2)
#endif //NS
	
	
		res |= rt5670_dsp_write(0x230c, 0x0100);//1_0x0200  Microphone volume. Range: 0~0x7fff. Unity gain: 0x100
		res |= rt5670_dsp_write(0x230d, 0x0080);//Speaker volume. Range: 0~0x7fff. Unity gain: 0x100
		res |= rt5670_dsp_write(0x2310, 0x0010);//Spkout 500Hz HPF
	
		/*---------------------Data path selection-------------------------*/
		res |= rt5670_dsp_write(0x22fc, 0x6000);
		res |= rt5670_dsp_write(0x229d, 0x0000);
		res |= rt5670_dsp_write(0x2260, 0x30d9);
		res |= rt5670_dsp_write(0x2260, 0x30d9);
	
		/* mic_pga */
		res |= rt5670_dsp_write(0x2278, 0xe0e0);
	
		res |= rt5670_dsp_write(0x2289, 0x7fff);
		res |= rt5670_dsp_write(0x2290, 0x7fff);
		res |= rt5670_dsp_write(0x2288, 0x0002);
		res |= rt5670_dsp_write(0x22b2, 0x0002);
		res |= rt5670_dsp_write(0x2295, 0x0001);
		res |= rt5670_dsp_write(0x22b3, 0x0001);
		res |= rt5670_dsp_write(0x22da, 0x0001);
		res |= rt5670_dsp_write(0x22fd, 0x001e);
		res |= rt5670_dsp_write(0x22c1, 0x1006);
		res |= rt5670_dsp_write(0x22c2, 0x1006);
		res |= rt5670_dsp_write(0x22c3, 0x1007);
		res |= rt5670_dsp_write(0x22c4, 0x1007);
		res |= rt5670_dsp_write(0x2261, 0x30d9);	// from StereoADC
		res |= rt5670_dsp_write(0x2282, 0x0008);	// from StereoADC
		res |= rt5670_dsp_write(0x2283, 0x0009);	// from StereoADC
		res |= rt5670_dsp_write(0x22d7, 0x0008);	// from StereoADC
		res |= rt5670_dsp_write(0x22d8, 0x0009);	// from StereoADC
		res |= rt5670_dsp_write(0x226c, 0x000c);	// sysclk 12.288Mhz
		res |= rt5670_dsp_write(0x226d, 0x000c);	// sysclk 12.288Mhz
		res |= rt5670_dsp_write(0x226e, 0x0022);	// sysclk 12.288Mhz
		res |= rt5670_dsp_write(0x22f2, 0x004c);
		res |= rt5670_dsp_write(0x2301, 0x0002);	// rate 16Khz
		res |= rt5670_dsp_write(0x2306, 0x063f);
		res |= rt5670_dsp_write(0x3fb5, 0x0000);
	
	
		/*---------------------Function selection-------------------------*/
		res |= rt5670_dsp_write(0x2303, 0x0330); //1_230  2_0x0600
		res |= rt5670_dsp_write(0x2304, 0x8332);	
		res |= rt5670_dsp_write(0x22fb, 0x0000);
#endif //end DSP setting


	res |= rt5670WriteReg(0x91, 0x0E06);
	res |= rt5670WriteReg(0x62, 0xDf01);
	res |= rt5670WriteReg(0x2A, 0x4646);
	res |= rt5670WriteReg(0x53, 0x3000);
	res |= rt5670WriteReg(0x8F, 0x3100);
	res |= rt5670WriteReg(0x8E, 0x8019);
	res |= rt5670WriteReg(0x03, 0x0808);
	res |= rt5670WriteReg(0x1B, 0x0033);
	/* stop all */
	rt5670Stop();
	return res;
}


/**
 * @brief Configure RT5670 I2S format
 *
 * @param fmt:            see Rt5670I2sFmt
 *
 * @return
 *     - (-1) Error
 *     - (0)  Success
 */
int rt5670ConfigFmt(Rt5670I2sFmt fmt)
{
    int res = 0;
    uint16_t reg = 0;

	res = rt5670ReadReg(RT5670_I2S1_SDP, &reg);
    reg = reg & ~RT5670_I2S_DF_MASK;
    res |= rt5670WriteReg(RT5670_I2S1_SDP, reg | fmt);

    return res;
}

/**
 * @param volume: DAC volume 0 ~ 100
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int rt5670SetVoiceVolume(int volume)
{
    int res = 0;

    return res;
}
/**
 *
 * @return
 *           volume
 */
int rt5670GetVoiceVolume(int *volume)
{
    int res = 0;
    uint8_t reg = 0;

    return res;
}

/**
 * @brief Configure RT5670 data sample bits
 *
 * @param bitPerSample:   see BitsLength
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int rt5670SetBitsPerSample(BitsLength bitPerSample)
{
    int res = 0;
    uint16_t reg = 0;
    int bits = (int)bitPerSample;

	res = rt5670ReadReg(RT5670_I2S1_SDP, &reg);
    reg = reg & ~RT5670_I2S_DL_MASK;
    res |= rt5670WriteReg(RT5670_I2S1_SDP, reg | (bits << 2));

    return res;
}

/**
 * @brief Configure RT5670 DAC mute or not. Basicly you can use this function to mute the output or don't
 *
 * @param enable: enable or disable
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int rt5670SetVoiceMute(int enable)
{
    int res;
    uint8_t reg = 0;

    return res;
}

int rt5670GetVoiceMute(void)
{
    int res = -1;
    uint8_t reg = 0;

	res = 0;
    return res == 0 ? reg : res;
}

/**
 * @param gain: Config DAC Output
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int rt5670ConfigDacOutput(DacOutput output)
{
    int res = 0;
    uint8_t reg = 0;

    return res;
}

/**
 * @param gain: Config ADC input
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int rt5670ConfigAdcInput(AdcInput input)
{
    int res;
    uint8_t reg = 0, data = 0x7F;

	data &= ~input;
	res |= rt5670_update_bits(RT5670_REC_L2_MIXER, 0x7F, input);
	res |= rt5670_update_bits(RT5670_REC_R2_MIXER, 0x7F, input);

    return res;
}

/**
 * @param gain: see MicBoostGain
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int rt5670SetMicBoostGain(AdcInput input, MicBoostGain gain)
{
    int res = 0;
    if (input & ADC_INPUT_IN1P)
    	res = rt5670_update_bits(RT5670_CJ_CTRL1, RT5670_CBJ_BST1_MASK,
    									gain<<RT5670_CBJ_BST1_SFT);
	if (input & ADC_INPUT_IN2P)
    	res = rt5670_update_bits(RT5670_IN2, RT5670_BST_MASK2,
    									gain<<RT5670_BST_SFT2);
	if (input & ADC_INPUT_IN3P)
    	res = rt5670_update_bits(RT5670_IN3, RT5670_BST_MASK1,
    									gain<<RT5670_BST_SFT1);
    return res;
}


