/** \file vs10xx.c
 * Functions for interfacing with the mp3 player chip.
 * \todo safe rewind
 * \todo VS1003 WMA "wma-bytes-left" variable adjustment at ff/rew
 */
#include "vs10xx.h"

/** Constructor of class VS1053. */
vs10xx::vs10xx(PinName MOSI, PinName MISO, PinName SCLK, PinName XCS, 
               PinName XDCS,PinName DREQ, PinName XRESET)
                :  
                _spi(MOSI,MISO,SCLK),
                _XCS(XCS),
                _XDCS(XDCS),
                _DREQ(DREQ),
                _XRESET(XRESET)    
{
    _XCS = 1;
    _XDCS = 1;
    _XRESET = 1;
}

/** Write the 16-bit value to VS10xx register*/
void vs10xx::writeRegister(unsigned char addressbyte,unsigned int value)
{
    _XCS = 1;
    while (!_DREQ);
    _XCS = 0;
    _spi.write(VS_WRITE_COMMAND);
    _spi.write(addressbyte);
    _spi.write(value >> 8);
    _spi.write(value & 0xFF);
    _XCS = 1;
}

/** Read the 16-bit value of a VS10xx register */
unsigned int vs10xx::readRegister (unsigned char addressbyte)
{
    unsigned int resultvalue = 0;
    
    _XCS = 1;
    while (!_DREQ);
    _XCS = 0;
    _spi.write(VS_READ_COMMAND);
    _spi.write((addressbyte));
    resultvalue = _spi.write(0XFF) << 8;
    resultvalue |= _spi.write(0XFF); 
    _XCS = 1;
    return resultvalue;
}

/** write data to VS10xx  */
void vs10xx::writeData(unsigned char *databuf,unsigned char n)
{
    _XDCS = 1;
    _XDCS = 0;
    while (!_DREQ);
    while (n--)
    {
        _spi.write(*databuf++);
    }
    _XDCS = 1;
}

void vs10xx::setFreq(int freq)
{
    _spi.frequency(freq);        //set freq for speed
}

void vs10xx::setVolume(unsigned char vol)
{
    writeRegister(SPI_VOL, vol*0x101);     //Set volume level
}

/** Soft Reset of VS10xx (Between songs) */
void vs10xx::softReset()
{
    _spi.frequency(1000000);          //low speed
    
    /* Soft Reset of VS10xx */
    writeRegister(SPI_MODE, 0x0804); /* Newmode, Reset, No L1-2 */
    
    wait_ms(2);         //delay
    while(!_DREQ);
    
    /* A quick sanity check: write to two registers, then test if we
     get the same results. Note that if you use a too high SPI
     speed, the MSB is the most likely to fail when read again. */
    writeRegister(SPI_HDAT0, 0xABAD);
    writeRegister(SPI_HDAT1, 0x1DEA);
    if (readRegister(SPI_HDAT0) != 0xABAD || readRegister(SPI_HDAT1) != 0x1DEA) {
        printf("There is something wrong with VS10xx\n");
    }
    
    writeRegister(SPI_CLOCKF,0XC000);   //Set the clock
    writeRegister(SPI_AUDATA,0xbb81);   //samplerate 48k,stereo
    writeRegister(SPI_BASS, 0x0055);    //set accent
    writeRegister(SPI_VOL, 0x1010);     //Set volume level
        
    while (!_DREQ);
    
}

/** Reset VS10xx */
void vs10xx::reset(){

    _XRESET = 0;
    wait_ms(2);  //it is a must
    
    /* Send dummy SPI byte to initialize SPI */
    _spi.write(0xFF);

    /* Un-reset VS10XX chip */
    _XCS = 1;
    _XDCS = 1;
    _XRESET = 1;

    softReset(); //vs10xx soft reset.

    printf("\r\nVS10xx Init\r\n");
}

/*     Loads a plugin.       */
void vs10xx::loadPlugin(const unsigned short *plugin,int length) {
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


