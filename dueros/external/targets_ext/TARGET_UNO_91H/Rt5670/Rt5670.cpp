/** \file rda58xx.c
 * Functions for interfacing with the codec chip.
 */
#include "Rt5670.h"
#include "Rt5670_interface.h"
#include "mbed_interface.h"

 /** Constructor of class RDA58XX. */
Rt5670::Rt5670()
{	
}

Rt5670::~Rt5670()
{	
}

void Rt5670::init()
{
	int ret = 0;
	int i = 0;
	uint8_t data = 1;
	Rt5670Config rt5670_init_conf = {
		.rtMode = RT_MODE_SLAVE,
		.adcInput = (AdcInput)(ADC_INPUT_IN1P | ADC_INPUT_IN2P | ADC_INPUT_IN3P),
		//.dacOutput = (DacOutput)(DAC_OUTPUT_LOUT1)
	};

	
	//for(int k = 0; k < 100000000; ++k) ;
	ret |= rt5670Init2();
	

	#if 0 //old_5670_setting
	ret |= rt5670Init(&rt5670_init_conf);
	

	printf("return value: %d when rt5670Init\r\n", ret);
	
	ret |= rt5670ConfigFmt(RT_I2S_NORMAL);
	printf("return value: %d when rt5670ConfigFmt\r\n", ret);
	
	
	ret |= rt5670SetBitsPerSample(BIT_LENGTH_16BITS);
	printf("return value: %d when rt5670SetBitsPerSample\r\n", ret);
	
	
	ret |= rt5670SetMicBoostGain(ADC_INPUT_IN1P, MIC_GAIN_20DB);
	printf("return value: %d when rt5670SetMicBoostGain of IN1P\r\n", ret);
	
	ret |= rt5670SetMicBoostGain(ADC_INPUT_IN3P, MIC_GAIN_20DB);
	printf("return value: %d when rt5670SetMicBoostGain of IN3P\r\n", ret);

	rt5670SetAdcDacVolume(RT_MODULE_ADC, 32);//JCP---2017/01/14: volume control	
	//I2sTx_Open();
#endif	
}

void Rt5670::start_record()
{	
	I2sRx_Open();	
	//I2sTx_Open();
}

void Rt5670::stop_record()
{
	I2sRx_Close();
	//I2sTx_Close();	
}

int Rt5670::stop_play() {
    return 0;
}


int Rt5670::write_data(void* data, int size)
{
	return I2s_Send_Data(data, size);
}

int Rt5670::read_data(void* data, int size)
{	
	return I2s_Recv_Data(data, size);
}

int Rt5670::read_done()
{	
	return I2s_Wait_RxDone();
}

int Rt5670::write_done()
{	
	return I2s_Wait_TxDone();
}


