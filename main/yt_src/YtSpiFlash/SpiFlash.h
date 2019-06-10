/*
 * File : SpiFLASH.h
 * Dscr : Headers for SPI RAM "IPS3204J".
 */

#ifndef SPI_RAM_H
#define SPI_RAM_H

#include "mbed.h"
#include "spi_api.h"

#define INVALID_BYTE    (0xFFU)
#define INVALID_WORD    (0xFFFFFFFFUL)

typedef enum {
    SPIFLASH_MODE_BYTE = 1,
    SPIFLASH_MODE_WORD = 4
} SPIFLASH_MODE_T;

typedef enum {
    SPIFLASH_RET_SUCCESS      = 0,
    SPIFLASH_RET_ERR_INPUT    = -1
} SPIFLASH_RET_CODE_T;

class SpiFLASH
{
public:
    /* Initialize and specify the SPI pins */
    SpiFLASH(PinName mosi, PinName miso, PinName sclk, PinName cs, SPIFLASH_MODE_T mode = SPIFLASH_MODE_WORD);

    /* Set SPIFLASH to byte/word mode */
    void setMode(SPIFLASH_MODE_T mode);

    uint8_t SPI_Flash_Write_Byte(uint8_t Data);

    void SPI_Flash_WriteEnable (void);

    void SPI_Flash_WriteDisable (void);

    void SPI_Flash_Wait_Busy(void);

    uint8_t SPI_Flash_Read_Busy(void);

    void SPI_Flash_Erase_Block( uint32_t Erase_Addr ,uint8_t mode );
	
	void SPI_Flash_Chip_Erase(void );

    void SPI_Flash_Read ( uint32_t ReadAddr , uint8_t* pBuffer , uint16_t NumByteToRead );
	
	void SPI_Flash_Write_Page (uint32_t WriteAddr, uint8_t* pBuffer , uint16_t NumByteToWrite);
	
	unsigned int SPI_Flash_ReadID(void);
	
    virtual void lock(void);

    /* Release exclusive access to this SpiFLASH device */
    virtual void unlock(void);

private:
    spi_t _spi;
    DigitalOut _cs;
    SPIFLASH_MODE_T _mode;
    static SingletonPtr<PlatformMutex> _mutex;

};

#endif