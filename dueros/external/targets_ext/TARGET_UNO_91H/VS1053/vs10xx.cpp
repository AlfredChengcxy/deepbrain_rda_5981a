/** \file vs10xx.c
 * Functions for interfacing with the mp3 player chip.
 * \todo safe rewind
 * \todo VS1003 WMA "wma-bytes-left" variable adjustment at ff/rew
 */
#include "vs10xx.h"
#include "spi_api.h"

/* Macros to swap byte order */
#define SWAP_BYTE_ORDER_WORD(val) ((((val) & 0x000000FF) << 24) + \
                                   (((val) & 0x0000FF00) << 8)  + \
                                   (((val) & 0x00FF0000) >> 8)   + \
                                   (((val) & 0xFF000000) >> 24))

spi_t vs1053_spi;

/** Constructor of class VS1053. */
vs10xx::vs10xx(PinName MOSI, PinName MISO, PinName SCLK, PinName XCS,
               PinName XDCS,PinName DREQ, PinName XRESET)
	:
	_XCS(XCS),
	_XDCS(XDCS),
	_DREQ(DREQ),
	_XRESET(XRESET)
{
	spi_init(&vs1053_spi, MOSI, MISO, SCLK, NC);
	spi_format(&vs1053_spi, 32, 0, 0);
	spi_frequency(&vs1053_spi, 1000000);
	_XCS = 1;
	_XDCS = 1;
	_XRESET = 1;
}

/** Write the 16-bit value to VS10xx register*/
void vs10xx::writeRegister(unsigned char addressbyte,unsigned int value)
{
	_XCS = 1;
	*((volatile unsigned long *)(0x4000105C)) = 0;

	while (!_DREQ) {
		printf("%d",int(_DREQ));
	}
	_XCS = 0;
	spi_master_write(&vs1053_spi, ((VS_WRITE_COMMAND << 24) | (addressbyte << 16) | (value & 0x0000FFFF)));
	_XCS = 1;
}

/** Read the 16-bit value of a VS10xx register */
unsigned int vs10xx::readRegister (unsigned char addressbyte)
{
	unsigned int resultvalue = 0;

	_XCS = 1;
	while (!_DREQ);
	_XCS = 0;
	resultvalue = spi_master_write(&vs1053_spi, ((VS_READ_COMMAND << 24) | (addressbyte << 16) | 0x0000FFFF));
	resultvalue &= 0x0000FFFF;
	_XCS = 1;
	return resultvalue;
}

/** write data to VS10xx  */
void vs10xx::writeDataU32(unsigned int *databuf,unsigned char n)
{
	unsigned int data;
	_XDCS = 1;
	_XDCS = 0;
	while (!_DREQ);
	while (n--) {
		data = SWAP_BYTE_ORDER_WORD(*databuf);
		spi_master_write(&vs1053_spi, data);
		databuf++;
	}
	_XDCS = 1;
}

/** write data to VS10xx  */
void vs10xx::writeDataU8(unsigned char *databuf,unsigned char n)
{
	spi_format(&vs1053_spi, 8, 0, 0);
	_XDCS = 1;
	_XDCS = 0;
	while (!_DREQ);
	while (n--) {
		spi_master_write(&vs1053_spi, *databuf++);
	}
	_XDCS = 1;
	spi_format(&vs1053_spi, 32, 0, 0);
}

/** write data to VS10xx  */
void vs10xx::writeData(unsigned char *databuf,unsigned char n)
{
#if 1
    writeDataU32((unsigned int *)databuf, n / 4);
    if (n % 4) {
        writeDataU8((unsigned char *)databuf + n / 4 * 4, n % 4);
    }
#else
	_XDCS = 1;
	_XDCS = 0;
	//printf("%d\r\n",int(_DREQ));
	while (!_DREQ);
	//printf(".\r\n");
	while (n--) {
		//_spi.write(*databuf++);
	}
	_XDCS = 1;
#endif
}


void vs10xx::setFreq(int freq)
{
	spi_frequency(&vs1053_spi, freq);     //set freq for speed
}

void vs10xx::setVolume(unsigned char vol)
{
	writeRegister(SPI_VOL, vol*0x101);     //Set volume level
}

/** Soft Reset of VS10xx (Between songs) */
void vs10xx::softReset()
{
	spi_frequency(&vs1053_spi, 1000000);         //low speed
	printf("SoftRst1\r\n");
	/* Soft Reset of VS10xx */
	writeRegister(SPI_MODE, 0x0804); /* Newmode, Reset, No L1-2 */
	printf("SoftRst2\r\n");

	wait_ms(2);         //delay
	while(!_DREQ);

	/* A quick sanity check: write to two registers, then test if we
	 get the same results. Note that if you use a too high SPI
	 speed, the MSB is the most likely to fail when read again. */
	writeRegister(SPI_HDAT0, 0xABAD);
	writeRegister(SPI_HDAT1, 0x1DEA);
	unsigned int reg_d0 = readRegister(SPI_HDAT0);
	unsigned int reg_d1 = readRegister(SPI_HDAT1);
	if (reg_d0 != 0xABAD || reg_d1 != 0x1DEA) {
		printf("There is something wrong with VS10xx, SPI_HDAT0=%08X, SPI_HDAT1=%08X\r\n", reg_d0, reg_d1);
	}
	/*
	if (readRegister(SPI_HDAT0) != 0xABAD || readRegister(SPI_HDAT1) != 0x1DEA) {
	    printf("There is something wrong with VS10xx\r\n");
	}
	*/

	writeRegister(SPI_CLOCKF,0XC000);   //Set the clock
	writeRegister(SPI_AUDATA,0xbb81);   //samplerate 48k,stereo
	writeRegister(SPI_BASS, 0x0055);    //set accent
	writeRegister(SPI_VOL, 0x1010);     //Set volume level

	while (!_DREQ);

}

/** Reset VS10xx */
void vs10xx::reset()
{

	_XRESET = 0;
	wait_ms(2);  //it is a must

	/* Send dummy SPI byte to initialize SPI */
	spi_format(&vs1053_spi, 8, 0, 0);
	spi_master_write(&vs1053_spi, 0xFF);
	spi_format(&vs1053_spi, 32, 0, 0);

	/* Un-reset VS10XX chip */
	_XCS = 1;
	_XDCS = 1;
	_XRESET = 1;

	printf("reset\r\n");
	softReset(); //vs10xx soft reset.

	printf("\r\nVS10xx Init\r\n");
}

/*     Loads a plugin.       */
void vs10xx::loadPlugin(const unsigned short *plugin,int length)
{
	int i = 0;
	while (i<length) {
		unsigned short addr, n, val;
		addr = plugin[i++];
		n = plugin[i++];
		if (n & 0x8000U) { /* RLE run, replicate n samples */
			n &= 0x7FFF;
			val = plugin[i++];
			while (n--) {
				writeRegister(addr, val);
			}
		} else {           /* Copy run, copy n samples */
			while (n--) {
				val = plugin[i++];
				writeRegister(addr, val);
			}
		}
	}
}




