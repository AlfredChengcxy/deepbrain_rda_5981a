#include "SpiLocal.h"
#include "SpiFlash.h"
#include "audio.h"

#define AUDIO_NUMS 20
#define FLASH_START_ADDR  0x000000//24BIT
#define FLASH_HEADER_LEN 0x400 // 256 *  用于存储头信息
#define FLASH_START_DATA_ADDR (FLASH_START_ADDR + FLASH_HEADER_LEN) // 
#define FLASH_PAGE_ALIGN_NUM 256
#define FLASH_MAX_ADDR 0x200000

#define ngx_align(d, a) (((d) + (a - 1)) & ~(a - 1))


struct AUDIO_ARRAY
{
	const char * pdata;
	int nlen;
};

struct AUDIO_HEADERS
{
	int total;
	struct AUDIO_HEAD
	{
		uint32_t addr;
		int index;
		int len;	
	}ah[AUDIO_NUMS];
};



static AUDIO_HEADERS ahs;
static SpiFLASH sFlash(PD_2, PD_3, PD_0, PD_1);




static bool gInit = false;
static int gCurentIndex = 0;



#define DEBUG_DATA 0

static void read_file_data(uint32_t addr,char * pdata,int nlen)
{
	/// assert (len % FLASH_PAGE_ALIGN_NUM = 0)
	int j =0;
	for( j = 0;j< nlen / FLASH_PAGE_ALIGN_NUM;j++)
	{
		sFlash.SPI_Flash_Read (addr + j * FLASH_PAGE_ALIGN_NUM, (uint8_t *)(pdata + j* FLASH_PAGE_ALIGN_NUM),FLASH_PAGE_ALIGN_NUM);

#if DEBUG_DATA/// just do test
		for(int i = 0;i< FLASH_PAGE_ALIGN_NUM / 16;i++)
		{
			printf("%08x:",addr + j * FLASH_PAGE_ALIGN_NUM + i * 16);
			for(int k = 0;k<16;k++)
				printf("0x%02x, ",*(pdata + j* FLASH_PAGE_ALIGN_NUM + i * 16 + k));
			printf("\r\n");
		}
#endif		
	}
	if(nlen % FLASH_PAGE_ALIGN_NUM != 0)
	{
		sFlash.SPI_Flash_Read (addr + j * FLASH_PAGE_ALIGN_NUM, (uint8_t *)(pdata + j* FLASH_PAGE_ALIGN_NUM),nlen % FLASH_PAGE_ALIGN_NUM);
#if DEBUG_DATA/// just do test
		int i = 0;
		int k = 0;	
		for( i = 0;i< (nlen % FLASH_PAGE_ALIGN_NUM) / 16;i++)
		{
			printf("%08x:",addr + j * FLASH_PAGE_ALIGN_NUM + i * 16);
			for( k = 0;k<16;k++)
				printf("0x%02x, ",*(pdata + j* FLASH_PAGE_ALIGN_NUM + i * 16 + k));
			printf("\r\n");
		}
		if((nlen % FLASH_PAGE_ALIGN_NUM % 16)!= 0)
		{
			printf("%08x:",addr + j * FLASH_PAGE_ALIGN_NUM + i * 16);		
			for( k = 0;k<nlen % FLASH_PAGE_ALIGN_NUM % 16;k++)
				printf("0x%02x, ",*(pdata + j* FLASH_PAGE_ALIGN_NUM + i * 16 + k));
			printf("\r\n");
		}
#endif		
	}	
}

static int read_file_header()
{
	uint32_t addr = FLASH_START_DATA_ADDR;
	sFlash.SPI_Flash_Read (FLASH_START_ADDR, (unsigned char *)&ahs,sizeof(ahs));
	printf("ahs.total = %d\r\n", ahs.total);
    for(int x = 0; x < ahs.total; x++) 
	{
        printf("ahs[%d].addr = %8x\r\n", (int)x, ahs.ah[x].addr);    
		printf("ahs[%d].index = %d\r\n", (int)x, ahs.ah[x].index);
		printf("ahs[%d].len = %d\r\n", (int)x, ahs.ah[x].len);
    }
	return ahs.total;
}



static void write_file()
{
	AUDIO_ARRAY audios[]=
	{	
#if 1	
		YT_LOCAL_ERGE,sizeof(YT_LOCAL_ERGE)/sizeof(char),
		YT_DB_WELCOME_PHC,sizeof(YT_DB_WELCOME_PHC)/sizeof(char),
		YT_LOCAL_GUSHI,sizeof(YT_LOCAL_GUSHI)/sizeof(char),
		YT_DB_WELCOME,sizeof(YT_DB_WELCOME)/sizeof(char),
		YT_LOCAL_GUOXUE,sizeof(YT_LOCAL_GUOXUE)/sizeof(char),
		YT_CONNTING_NET,sizeof(YT_CONNTING_NET)/sizeof(char),
		YT_LOCAL_YINGYU,sizeof(YT_LOCAL_YINGYU)/sizeof(char),
		YT_OPEN_BT,sizeof(YT_OPEN_BT)/sizeof(char)
#else	//// 最大测试
		YT_DB_WIFI_CONNECTING_TONE_LONG,sizeof(YT_DB_WIFI_CONNECTING_TONE_LONG)/sizeof(char),
		YT_DB_WIFI_CONNECTING_TONE_LONG,sizeof(YT_DB_WIFI_CONNECTING_TONE_LONG)/sizeof(char),
		YT_DB_WIFI_CONNECTING_TONE_LONG,sizeof(YT_DB_WIFI_CONNECTING_TONE_LONG)/sizeof(char),
		YT_DB_WIFI_CONNECTING_TONE_LONG,sizeof(YT_DB_WIFI_CONNECTING_TONE_LONG)/sizeof(char),
		YT_DB_WIFI_CONNECTING_TONE_LONG,sizeof(YT_DB_WIFI_CONNECTING_TONE_LONG)/sizeof(char),
		YT_DB_WIFI_CONNECTING_TONE_LONG,sizeof(YT_DB_WIFI_CONNECTING_TONE_LONG)/sizeof(char),
		YT_DB_WIFI_CONNECTING_TONE_LONG,sizeof(YT_DB_WIFI_CONNECTING_TONE_LONG)/sizeof(char),
		YT_DB_WIFI_CONNECTING_TONE_LONG,sizeof(YT_DB_WIFI_CONNECTING_TONE_LONG)/sizeof(char),
		YT_DB_WIFI_CONNECTING_TONE_LONG,sizeof(YT_DB_WIFI_CONNECTING_TONE_LONG)/sizeof(char),
		YT_DB_WIFI_CONNECTING_TONE_LONG,sizeof(YT_DB_WIFI_CONNECTING_TONE_LONG)/sizeof(char),
#endif
	};

	int audio_len = sizeof(audios)/sizeof(audios[0]);
	audio_len = audio_len > AUDIO_NUMS? AUDIO_NUMS : audio_len;
	
	ahs.total = audio_len;
	uint32_t addr = FLASH_START_DATA_ADDR;
	printf("SPI_Flash_Chip_Erase begin\r\n");
	sFlash.SPI_Flash_Chip_Erase();
	printf("SPI_Flash_Chip_Erase finish\r\n");
	
	for(int i = 0;i< ahs.total;i++)
	{
		if(addr + audios[i].nlen > FLASH_MAX_ADDR )
		{
			printf("flash can't write too much audio data\r\n");
			printf("addr:[%x]\r\n",addr);
			printf("next audios nlen:[%d]\r\n",audios[i].nlen);
			printf("next addr:[%x]\r\n",addr + audios[i].nlen);
			printf("max addr:[%x]\r\n",FLASH_MAX_ADDR);
			printf("true write total songs:[%d]\r\n",i);
			ahs.total = i;
			break;
		}
		ahs.ah[i].addr = addr;	
		ahs.ah[i].index = i;
		ahs.ah[i].len = audios[i].nlen;
		addr += ngx_align(ahs.ah[i].len,FLASH_PAGE_ALIGN_NUM) ;// 下一个audio地址	
	}
	sFlash.SPI_Flash_Write_Page (FLASH_START_ADDR, (unsigned char *)&ahs,sizeof(ahs));

	printf("ahs.total:[%d]\r\n",ahs.total);
	for(int i = 0;i< ahs.total;i++)
	{
		uint32_t addr = ahs.ah[i].addr;
		int len = ahs.ah[i].len;
		printf("i:[%d]\r\n",i);
		printf("ahs.addr:[%x]\r\n",ahs.ah[i].addr);
		printf("ahs.len:[%d]\r\n",ahs.ah[i].len);
	#if 0	
		for(int j = 0;j< ngx_align(ahs.ah[i].len,FLASH_PAGE_ALIGN_NUM) / FLASH_PAGE_ALIGN_NUM;j++)
		{
			sFlash.SPI_Flash_Write_Page (addr + j * FLASH_PAGE_ALIGN_NUM, (uint8_t *)(audios[i].pdata + j* FLASH_PAGE_ALIGN_NUM),FLASH_PAGE_ALIGN_NUM);
		}
	#else
		int j = 0;
		for( j = 0;j< len / FLASH_PAGE_ALIGN_NUM;j++)
		{
			sFlash.SPI_Flash_Write_Page (addr + j * FLASH_PAGE_ALIGN_NUM, (uint8_t *)(audios[i].pdata + j* FLASH_PAGE_ALIGN_NUM),FLASH_PAGE_ALIGN_NUM);
		}
		sFlash.SPI_Flash_Write_Page (addr + j * FLASH_PAGE_ALIGN_NUM, (uint8_t *)(audios[i].pdata + j* FLASH_PAGE_ALIGN_NUM),len - j * FLASH_PAGE_ALIGN_NUM);
	#endif
	}	

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**************************************************************************************************************************/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void spi_local_init()
{
	if(gInit == true) return;
#if 1   //// 优先判断是否写入,如已经写入信息则不必写入
	int num = read_file_header();
	printf("num:[%d]\r\n",num);
	if(num == 0)write_file();
#else	//// 每次强制写入
	write_file();
	read_file_header();
#endif
	gInit = true;
	gCurentIndex = 0;
}

void spi_local_getnext(uint32_t *paddr,int *nlen)
{
	if(!gInit)return;
	*paddr = ahs.ah[gCurentIndex].addr;
	*nlen = ahs.ah[gCurentIndex].len;
#if 1//DEBUG_DATA
	printf("addr:[%x]\r\n",*paddr);
	printf("index:[%d],gCurentIndex:[%d]\r\n",ahs.ah[gCurentIndex].index,gCurentIndex);
	printf("len:[%d]\r\n",*nlen);
#endif	
	gCurentIndex++;
	gCurentIndex %= ahs.total;
}

void spi_local_getpre(uint32_t *paddr,int *nlen)
{
	if(!gInit)return;
	*paddr = ahs.ah[gCurentIndex].addr;
	*nlen = ahs.ah[gCurentIndex].len;
#if 1//DEBUG_DATA	
	printf("addr:[%x]\r\n",*paddr);
	printf("index:[%d],gCurentIndex:[%d]\r\n",ahs.ah[gCurentIndex].index,gCurentIndex);
	printf("len:[%d]\r\n",*nlen);	
#endif	
	gCurentIndex--;
	if(gCurentIndex < 0)gCurentIndex = ahs.total -1;
}

void spi_local_get_frame(uint32_t  addr,char *pdata,int nlen)
{
	if(!gInit)return;
	read_file_data(addr,pdata,nlen);
}


