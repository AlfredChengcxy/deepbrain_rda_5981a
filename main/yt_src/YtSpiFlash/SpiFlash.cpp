/*
 * File : SpiFLASH.c
 * Dscr : Functions for SPI RAM "IPS3204J".
 */
#include "SpiFLASH.h"

#include "mbed.h"
#include "mbed_interface.h"
/* Macros */
//#define TOGGLE_SPI_ENDIANNESS

#define FLASH_SPI_FREQ        20000000
#define FLASH_PAGE_SIZE       256U

#define W25X_BUSY       0
#define W25X_NotBUSY    1   
#define Dummy_Byte1     0xFF

/*********************************************
- W25X的操作指令表，MCU通过向W25X
  发送以下指令就可以对W25X进行以下的操作
*********************************************/
#define W25X_WriteEnable            0x06    //Write Enable
//#define W25X_WriteEnableVSR         0x50    //Write Enable for Volatile Status Register
#define W25X_WriteDisable           0x04    //Write Disable

#define W25X_ReadStatusReg1         0x05    //读状态寄存器1：S7~S0
//#define W25X_ReadStatusReg2         0x35    //读状态寄存器2：S15~S8
#define W25X_WriteStatusReg         0x01    //写读状态寄存器：BYTE1:S7~S0  BYTE2：S15~S8

#define W25X_PageProgram            0x02    //单页编程：BYTE1:A23~A16  BYTE2:A15~A8  BYTE3:A7~A0  BYTE4:D7~D0

#define W25X_SectorErase            0x20    //扇区擦除：4K  BYTE1:A23~A16  BYTE2:A15~A8  BYTE3:A7~A0
#define W25X_BlockErase32K          0x52    //块擦除：32K  BYTE1:A23~A16  BYTE2:A15~A8  BYTE3:A7~A0
#define W25X_BlockErase64K          0xD8    //块擦除：64K  BYTE1:A23~A16  BYTE2:A15~A8  BYTE3:A7~A0
#define W25X_ChipErase              0xC7    //芯片擦除

//#define W25X_EraseSuspend           0x75    //暂停擦除
//#define W25X_EraseResume            0x7A    //恢复擦除
//#define W25X_ContinuousReadMode     0xFF    //连续读模式
#define W25X_ReadData               0x03    //读数据：BYTE1:A23~A16  BYTE2:A15~A8  BYTE3:A7~A0  BYTE4:D7~D0

#define W25X_FastReadData           0x0B    //快速读取：BYTE1:A23~A16  BYTE2:A15~A8  BYTE3:A7~A0  BYTE4:dummy  BYTE5:D7~D0
#define W25X_FastReadDual           0x3B    //快速双读取：BYTE1:A23~A16  BYTE2:A15~A8  BYTE3:A7~A0  BYTE4:dummy  BYTE5:D7~D0

#define W25X_PowerDown              0xB9    //掉电
#define W25X_ReleasePowerDown       0xAB   
#define W25X_DeviceID               0xAB

#define W25X_ManufactDeviceID       0x90
#define W25X_JedecDeviceID          0x9F

#if 0
#define SPI_MOSI_PIN  GPIO_PIN0 
#define SPI_CLK_PIN   GPIO_PIN9                                                  
#define SPI_CS_PIN    GPIO_PIN7 
#define SPI_MISO_PIN  GPIO_PIN6
#else
#define SPI_MOSI_PIN  GPIO_PIN6 
#define SPI_CLK_PIN   GPIO_PIN23                                                  
#define SPI_CS_PIN    GPIO_PIN3 
#define SPI_MISO_PIN  GPIO_PIN25 
#endif
//#if SPI_GPIO_USED


#if 1
static mbed::DigitalOut SPI_MOSI(SPI_MOSI_PIN,0);
static mbed::DigitalOut SPI_CLK(SPI_CLK_PIN,0);
static mbed::DigitalOut SPI_CS(SPI_CS_PIN,1);
static mbed::DigitalIn SPI_MISO(SPI_MISO_PIN);
#else
static mbed::DigitalOut SPI_MOSI(NC,0);
static mbed::DigitalOut SPI_CLK(NC,0);
static mbed::DigitalOut SPI_CS(NC,1);
static mbed::DigitalIn SPI_MISO(NC);

#endif

//#endif

SingletonPtr<PlatformMutex> SpiFLASH::_mutex;

/* Constructor */
SpiFLASH::SpiFLASH(PinName mosi, PinName miso, PinName sclk, PinName cs, SPIFLASH_MODE_T mode):
        _cs(cs), _mode(mode)
{

#if 0
    spi_init(&_spi, mosi, miso, sclk, NC);
    spi_format(&_spi, 8, 1, 0);  // ensure 8-bit mode before resetDevice()
    spi_frequency(&_spi, FLASH_SPI_FREQ);
#endif	
	/*
    resetDevice();
    if(_mode == SPIFLASH_MODE_WORD) {
        spi_format(&_spi, 32, 1, 0);
    }
	*/
	
}

void SpiFLASH::setMode(SPIFLASH_MODE_T mode)
{
    int bits;
    _mode = mode;
	/*
    bits = (_mode == SPIFLASH_MODE_WORD) ? 32 : 8;
    spi_format(&_spi, bits, 1, 0);
	*/
	spi_format(&_spi, 8, 1, 0);
}

/***************************************************************************
    函 数 名：SPI_Flash_Write_Byte
    功    能：软SPI_Flash 总线驱动基础函数，发送单个字节到MOSI，并同时接受MISO数据
    参    数：unsigned char Data MOSI数据线上发送的数据
    返    回：unsigned char Out         MISO数据线上接受的数据
    ****************************************************************************/
uint8_t SpiFLASH::SPI_Flash_Write_Byte(uint8_t Data)
{
	uint8_t Out = 0;

    uint8_t i;
	for(i = 8; i > 0; i--)
	{
		SPI_CLK = 0;
		SPI_MOSI=Data>>7;
		Data <<= 1;
		
		__ASM("NOP");
		__ASM("NOP");
		__ASM("NOP");
		__ASM("NOP");
		__ASM("NOP");
		
		Out<<=1;
		SPI_CLK = 1;
		Out|=SPI_MISO;
	}
	SPI_CLK = 0;		
	//Out= (uint8_t) spi_master_write(&_spi, (int) Data);							
	return Out;
}
//*************** 写使能 ****************************  OK
void SpiFLASH::SPI_Flash_WriteEnable (void)
{
	SPI_CS = 0;	
    __ASM("NOP");
    __ASM("NOP");	
	SPI_Flash_Write_Byte(W25X_WriteEnable);
    __ASM("NOP");
    __ASM("NOP");    
	SPI_CS = 1;
}

//*************** 写禁止 ****************************    OK
void SpiFLASH::SPI_Flash_WriteDisable (void)
{
	SPI_CS = 0;	
	__ASM("NOP");
	__ASM("NOP");
	SPI_Flash_Write_Byte(W25X_WriteDisable);
    __ASM("NOP");
    __ASM("NOP");	
	SPI_CS = 1;	
}


/***************BUSY等待*********************************************
函 数 名：SPI_Flash_Wait_Busy
功    能：读FLASH的BUSY，如果忙就等待。BUSY的原因是擦除，或是连续读写
*********************************************************************/
void SpiFLASH::SPI_Flash_Wait_Busy(void)
{                                
	uint8_t s;        
	SPI_CS = 0;
	do
	{
		SPI_Flash_Write_Byte(W25X_ReadStatusReg1);
		s = SPI_Flash_Write_Byte(Dummy_Byte1);
		//  连续读StatusReg1，不影响总操作时间，也就是芯片不会添忙
	}
	while ((s&0x01)==0x01);
	//while (1);
	SPI_CS = 1;
}
/***************读BUSY*********************************************
函 数 名：SPI_Flash_Read_Busy
功    能：读FLASH的BUSY
返    回：空返回NotBUSY，忙返回BUSY
*********************************************************************/
uint8_t SpiFLASH::SPI_Flash_Read_Busy(void)
{                                
	uint8_t s;        
	SPI_CS = 0;
	SPI_Flash_Write_Byte(W25X_ReadStatusReg1);
	s = SPI_Flash_Write_Byte(Dummy_Byte1);
	SPI_CS = 1;
	if(s&0x01)return (W25X_BUSY);
	else return (W25X_NotBUSY);
}
/************************************************************************
函 数 名：SPI_Flash_Erase_Block
功能描述: 擦除一个32K or 64K 块
输入参数: u32 Data_Addr 开始擦除的块首地址
mode        擦除模式 1=32K 其他=64K
************************************************************************/
void SpiFLASH::SPI_Flash_Erase_Block( uint32_t Erase_Addr ,uint8_t mode )
{
	SPI_Flash_WriteEnable();
	SPI_Flash_Wait_Busy();
	SPI_CS = 0;
	if(mode==1){SPI_Flash_Write_Byte(W25X_BlockErase32K);}  //根据定义，发送32K或64K擦除指令
	else {SPI_Flash_Write_Byte(W25X_BlockErase64K);}
	SPI_Flash_Write_Byte(Erase_Addr >> 16);
	SPI_Flash_Write_Byte(Erase_Addr >> 8);
	SPI_Flash_Write_Byte(Erase_Addr);
	SPI_CS = 1;
	SPI_Flash_Wait_Busy();                //等待擦除完成
}

void SpiFLASH::SPI_Flash_Chip_Erase(void )
{
	SPI_Flash_WriteEnable();
	SPI_Flash_Wait_Busy();
	SPI_CS = 0;
	SPI_Flash_Write_Byte(W25X_ChipErase);
	SPI_CS = 1;
	SPI_Flash_Wait_Busy();                //等待擦除完成
}


/************************************************************************
函 数 名  : SPI_Flash_Read
功能描述  : 在指定地址开始读取指定长度的数据
输入参数  : u32 ReadAddr       开始读取的地址(24bit)
u8* pBuffer        数据存储区
u16 NumByteToRead  要读取的字节数(最大65535)
*****************************************************************************/
void SpiFLASH::SPI_Flash_Read ( uint32_t ReadAddr , uint8_t* pBuffer , uint16_t NumByteToRead )
{
	uint16_t i;
	SPI_CS = 0;/* Enable chip select */
	SPI_Flash_Write_Byte(W25X_ReadData);
	SPI_Flash_Write_Byte(ReadAddr >> 16);
	SPI_Flash_Write_Byte(ReadAddr >> 8);
	SPI_Flash_Write_Byte(ReadAddr);
	for(i=0;i<NumByteToRead;i++)
	{
		pBuffer[i] = SPI_Flash_Write_Byte(Dummy_Byte1); //Read one byte
	}
	SPI_CS = 1;/* Disable chip select */
}
/*************************************************************************
函 数 名：SPI_Flash_Write_Page
功    能：SPI在一页(0~65535)的指定地址开始写入最大256字节的数据
输入参数：pBuffer:数据存储区
WriteAddr:开始写入的地址(24bit)
NumByteToWrite:要写入的字节数(最大256),该数不应该超过该页的剩余字节数!!!
***************************************************************************/
void SpiFLASH::SPI_Flash_Write_Page (uint32_t WriteAddr, uint8_t* pBuffer , uint16_t NumByteToWrite)
{
	uint16_t i;  
	SPI_Flash_WriteEnable();                  //SET WEL
	SPI_CS = 0;
	SPI_Flash_Write_Byte(W25X_PageProgram);      //发送写页命令   
	SPI_Flash_Write_Byte((uint8_t)((WriteAddr)>>16)); //发送24bit地址   
	SPI_Flash_Write_Byte((uint8_t)((WriteAddr)>>8));   
	SPI_Flash_Write_Byte((uint8_t)WriteAddr);   
	for(i=0;i<NumByteToWrite;i++)SPI_Flash_Write_Byte(pBuffer[i]);//循环写数  
	SPI_CS = 1;                                          //取消片选
	SPI_Flash_Wait_Busy();                  //等待写入结束
	SPI_Flash_WriteDisable();
}

/*******************************************************************************
* Function Name  : SPI_FLASH_ReadID
* Description    : Reads FLASH identification.
* Input          : None
* Output         : None
* Return         : FLASH identification
函 数 SPI_Flash_ReadID
功    能：读FLASH的ID
输入参数：无
返回：设备的识别号
*******************************************************************************/
unsigned int SpiFLASH::SPI_Flash_ReadID(void)
{
	uint16_t        Temp = 0;
	/* Enable chip select */
	SPI_CS = 0;
	/* Send "RDID " instruction */
	SPI_Flash_Write_Byte(W25X_ManufactDeviceID);
	SPI_Flash_Write_Byte(0x00);
	SPI_Flash_Write_Byte(0x00);
	SPI_Flash_Write_Byte(0x00);
	/* Read a byte from the FLASH */
	Temp|= SPI_Flash_Write_Byte(Dummy_Byte1)<<8;
	Temp|= SPI_Flash_Write_Byte(Dummy_Byte1);
	/* Disable chip select */
	SPI_CS = 1;
	return Temp;
}


void SpiFLASH::lock()
{
    _mutex->lock();
}

void SpiFLASH::unlock()
{
    _mutex->unlock();
}

