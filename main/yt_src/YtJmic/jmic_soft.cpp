#include "mbed.h"
//#include "I2C.h"
#include "gpio_api.h"
#include "DX8_API.h"


//条件编译 1:使用软件模拟I2C  

static gpio_t dx8I2C_SCL;
static gpio_t dx8I2C_SDA;

typedef unsigned char  u8;
typedef __IO uint32_t  vu32;

static __inline void TWI_SCL_0(void)        { gpio_write(&dx8I2C_SCL,0); } 
static __inline void TWI_SCL_1(void)        { gpio_write(&dx8I2C_SCL,1); } 
static __inline void TWI_SDA_0(void)        { gpio_write(&dx8I2C_SDA,0); } 
static __inline void TWI_SDA_1(void)        { gpio_write(&dx8I2C_SDA,1); } 
static __inline u8   TWI_SDA_STATE(void)    { return gpio_read(&dx8I2C_SDA); }

static __inline void TWI_SDA_INPUT(void)    { gpio_dir(&dx8I2C_SDA,PIN_INPUT); }
static __inline void TWI_SDA_OUTPUT(void)   { gpio_dir(&dx8I2C_SDA,PIN_OUTPUT); }
  
/******************************************************************************* 
 * 函数名称:TWI_Initialize                                                                      
 * 描    述:I2C初始化函数                                                                      
 *                                                                                
 * 输    入:无                                                                      
 * 输    出:无                                                                      
 * 返    回:无                                                                      
 * 作    者:                                                                      
 * 修改日期:2010年6月8日                                                                     
 *******************************************************************************/  
void dx8_Init(void)  
{ 

/* I2C Pins: sda, scl */
//mbed::I2C i2c(GPIO_PIN22, GPIO_PIN23);
//static mbed::I2C i2c(PD_0, PD_1);


#if 0
  gpio_init(&dx8I2C_SCL,PD_0);  
  gpio_init(&dx8I2C_SDA,PD_1);
#else
  gpio_init(&dx8I2C_SCL,PD_1);  
  gpio_init(&dx8I2C_SDA,PD_0);
#endif


  TWI_SCL_1();
	TWI_SDA_1();
  gpio_dir(&dx8I2C_SCL,PIN_OUTPUT);
  gpio_dir(&dx8I2C_SDA,PIN_OUTPUT);
	TWI_SCL_1();
	TWI_SDA_1();
} 

/******************************************************************************* 
 * 函数名称:TWI_Delay                                                                      
 * 描    述:延时函数                                                                      
 *                                                                                
 * 输    入:无                                                                      
 * 输    出:无                                                                      
 * 返    回:无                                                                      
 * 作    者:                                                                      
 * 修改日期:2010年6月8日                                                                     
 *******************************************************************************/  
static void TWI_NOP(void)  
{  
		vu32 i, j;  
		vu32 sum = 0;  
		i = 2;  
		while(i--)  
		{  
			 for (j = 0; j < 2; j++)  
			 sum += i;   
		}  
		sum = i;  
}
  
/******************************************************************************* 
 * 函数名称:TWI_START                                                                      
 * 描    述:发送启动                                                                      
 *                                                                                
 * 输    入:无                                                                      
 * 输    出:无                                                                      
 * 返    回:无                                                                      
 * 作    者:                                                                      
 * 修改日期:2010年6月8日                                                                     
 *******************************************************************************/  
static void TWI_START(void)  
{   
		TWI_SDA_1();   
		TWI_NOP();  
			 
		TWI_SCL_1();   
		TWI_NOP();      

		TWI_SDA_0();  
		TWI_NOP();  
			
		TWI_SCL_0();    
		TWI_NOP(); 
}  
  
/* --------------------------------------------------------------------------*/  
/**  
 * @Brief:  TWI_STOP  
 */  
/* --------------------------------------------------------------------------*/  
static void TWI_STOP(void)  
{  
		TWI_SDA_0();   
		TWI_NOP();  
			 
		TWI_SCL_1();   
		TWI_NOP();      

		TWI_SDA_1();  
		TWI_NOP();   
}  
  
/* --------------------------------------------------------------------------*/  
/**  
 * @Brief:  TWI_SendACK  
 */  
/* --------------------------------------------------------------------------*/  
static void TWI_SendACK(void)  
{  
		TWI_SDA_0();  
		TWI_NOP();  
		TWI_SCL_1();  
		TWI_NOP();  
		TWI_SCL_0();   
		TWI_NOP();  
}  
  
/* --------------------------------------------------------------------------*/  
/**  
 * @Brief:  TWI_SendNACK  
 */  
/* --------------------------------------------------------------------------*/  
static void TWI_SendNACK(void)  
{  
		TWI_SDA_1();  
		TWI_NOP();  
		TWI_SCL_1();  
		TWI_NOP();
		TWI_SCL_0();  
		TWI_NOP();    
}  
  
/* --------------------------------------------------------------------------*/  
/**  
 * @Brief:  TWI_SendByte  
 *  
 * @Param: Data 
 *  
 * @Returns:    
 */  
/* --------------------------------------------------------------------------*/  
static u8 TWI_SendByte(u8 Data)  
{  
		u8 i,ret;

		for(i = 0; i < 8; i++)
		{    
				//---------数据建立----------  
				if(Data & 0x80)  
				{  
						TWI_SDA_1(); 
				}  
				else  
				{  
						TWI_SDA_0();  
				}   
				Data<<=1;  
				TWI_NOP(); 

				//---数据建立保持一定延时----    
				//----产生一个上升沿[正脉冲]   
				TWI_SCL_1();  
				TWI_NOP();  
				TWI_SCL_0();  
				TWI_NOP();//延时,防止SCL还没变成低时改变SDA,从而产生START/STOP信号  
				//---------------------------     
		} 

		//接收从机的应答   
		TWI_SDA_INPUT();
		TWI_NOP();
		
		TWI_SCL_1();  
		TWI_NOP();
		ret = TWI_SDA_STATE();
		TWI_NOP();
		TWI_SCL_0();
		TWI_NOP();
	
		TWI_NOP();
		TWI_NOP();
		TWI_SDA_OUTPUT();			
		TWI_NOP();		
				  
    if (ret)
		{
				return 1; 
		}
		else  
		{  
				return 0;    
		}      
}  
  
/* --------------------------------------------------------------------------*/  
/**  
 * @Brief:  TWI_ReceiveByte  
 *  
 * @Returns:    
 */  
/* --------------------------------------------------------------------------*/  
static u8 TWI_ReceiveByte(void)  
{  
		u8 i,Dat;

		TWI_SDA_INPUT();
    TWI_NOP();	
  
		Dat = 0;  
		for(i = 0; i < 8; i++)  
		{  
				TWI_SCL_1();//产生时钟上升沿[正脉冲],让从机准备好数据   
				TWI_NOP();   
				Dat<<=1;
			
				if(TWI_SDA_STATE()) //读引脚状态  
				{  
				 Dat |= 0x01;   
				}
				
				TWI_SCL_0();//准备好再次接收数据    
				TWI_NOP();//等待数据准备好           
		}

		TWI_NOP();
		TWI_NOP();
		TWI_SDA_OUTPUT();
		TWI_NOP();
 
		return Dat;  
}  

unsigned char dxif_transfer(unsigned char *buf, unsigned short len)
{
		unsigned short i;
		
		dx8_Init();

		// Start
		TWI_START();

		// slave address
		if (0 != TWI_SendByte(buf[0])) 
		{
				TWI_STOP();
				return 1;
		}

		// Data
		if (buf[0] & 0x01)  // i2c read
		{
				for (i = 1; i < len; i++) 
				{
					buf[i] = TWI_ReceiveByte();
					if (i == (len - 1)) 
					{
							TWI_SendNACK();
					}
					else 
					{
							TWI_SendACK();
					}
				}
		}
		else  // i2c write
		{		
				for (i = 1; i < len; i++) 
				{
						if (0 != TWI_SendByte(buf[i]))
						{
								TWI_STOP();
								return 1;
						}
				}
		}

		// Stop
		TWI_STOP();

		return 0;
}
