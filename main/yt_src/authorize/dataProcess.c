//
//  dataProcess.c
//  protocol 1.0
//
//  Created by ustudy on 19/1/14.
//  Copyright ustudy. All rights reserved.
//

//97 bytes for one packet


#include <string.h>
#include "dataProcess.h"

#define DEFAULT_CODE "ytkjcode" 

#define KEY_ACTIVATION_CODE 'K'
#define KEY_CONNECTION_TIME 'C'
#define KEY_DATE 'D'
#define KEY_MAC 'W'

#define KEY_CUSTOMER 'M'
#define KEY_PRODUCT 'R'


static void you_333_GenerateCRC(char *pPacket, int nPacketLen, char *pRes_5Bytes)
{
	int i = 0, j;
	unsigned char p5Bytes[5] = {0x37, 0x29, 0xf3,0xe4,0x85};
	if(97 == nPacketLen)
	{

		for(j=0; j<5;j++)pRes_5Bytes[j] = p5Bytes[j];

		i = 0;
		while(i < 90)
		{
			for(j=0; j<5;j++)pRes_5Bytes[j] ^= pPacket[i+j];
			i += 5;
		}
	}
	else for(j=0; j<5;j++)pRes_5Bytes[j] = 0x00;
}

static void you_333_ModifyInfo(char *strCode, unsigned char charKey, unsigned int nCodeLen, char *strNewCode)
{
	unsigned int i = 0;
	unsigned char charOne;

	for(i=0; i<nCodeLen; i++)
	{
		charOne = (unsigned char)(i%256);
		charOne ^= charKey;
		charOne ^= strCode[i];
		strNewCode[i] = charOne;
	}
}


//it returns 0 is server is abnoraml
//otherwise return 1
static int you_333_CheckServerInfo_001(char *pDecryptedServerPacket)
{
    return (pDecryptedServerPacket[3] == '0')?-1:0;
}

//it returns 0 if no license is available
//it returns 1 if license is available
static int you_333_CheckServerInfo_002(char *pDecryptedServerPacket)
{
    return (pDecryptedServerPacket[36] == '0')?-1:0;
}

//it returns 0 if user id, product id, activation or connection time is incorrect
//otherwise it returns 1
static int you_333_CheckServerInfo_003(char *pDecryptedServerPacket)
{
    return (pDecryptedServerPacket[37] == '0')?-1:0;
}

//it returns  0 if upperbound times are reached
//it returns 1 in other cases
static int you_333_CheckServerInfo_004(char *pDecryptedServerPacket)
{
    return (pDecryptedServerPacket[38] == '0')?-1:0;
}

//get connection times
static void you_333_GetServerTimes(char *pDecryptedServerPacket, char *pConnectTimes, int *pByteNumber)
{
    memset(pConnectTimes, 0, 8);
    memcpy(pConnectTimes, pDecryptedServerPacket + 39, 8);
	*pByteNumber = 8;
}

//get activation code
static void you_333_GetServerCode(char *pDecryptedServerPacket, char *pActivationCode, int *pByteNumber)
{
    memset(pActivationCode, 0, 8);
    memcpy(pActivationCode, pDecryptedServerPacket + 28, 8);
	*pByteNumber = 8;
}


static const unsigned short pForwardTable[PACKET_BYTE_NUM] = 
{
	47, 50, 77, 52, 89, 60, 78, 45, 9, 4, 81, 8, 73, 46, 93, 55, 
	31, 36, 28, 61, 0, 25, 90, 21, 15, 39, 18, 22, 56, 91, 11, 23, 
	57, 19, 53, 6, 44, 58, 79, 82, 70, 16, 7, 5, 37, 80, 49, 32, 
	64, 2, 75, 12, 94, 26, 67, 51, 34, 74, 85, 76, 24, 14, 69, 92, 
	95, 38, 43, 84, 54, 27, 33, 35, 63, 48, 96, 66, 13, 10, 65, 88, 
	72, 17, 71, 62, 20, 68, 1, 86, 30, 42, 41, 87, 40, 3, 59, 83, 
	29
};


static int you_333_TranPacketForward(char *pInputPacket, int nPacketSize, char *pOutputPacket)
{
	unsigned short nNewPos,i;

	if(PACKET_BYTE_NUM == nPacketSize)
	{
		for(i=0; i<PACKET_BYTE_NUM;i++)
		{
			nNewPos = pForwardTable[i];
			pOutputPacket[nNewPos] = pInputPacket[i];
		}

		return 0;
	}
	
	return -1;
}

static const unsigned short pBackwardTable[PACKET_BYTE_NUM] = 
{
	20, 86, 49, 93, 9, 43, 35, 42, 11, 8, 77, 30, 51, 76, 61, 24, 
	41, 81, 26, 33, 84, 23, 27, 31, 60, 21, 53, 69, 18, 96, 88, 16, 
	47, 70, 56, 71, 17, 44, 65, 25, 92, 90, 89, 66, 36, 7, 13, 0, 
	73, 46, 1, 55, 3, 34, 68, 15, 28, 32, 37, 94, 5, 19, 83, 72, 
	48, 78, 75, 54, 85, 62, 40, 82, 80, 12, 57, 50, 59, 2, 6, 38, 
	45, 10, 39, 95, 67, 58, 87, 91, 79, 4, 22, 29, 63, 14, 52, 64, 
	74
};

static int you_333_TranPacketBackward(char *pInputPacket, int nPacketSize, char *pOutputPacket)
{
	unsigned short nNewPos,i,j;

	if(PACKET_BYTE_NUM == nPacketSize)
	{
		for(i=0; i<PACKET_BYTE_NUM;i++)
		{
			nNewPos = pBackwardTable[i];
			pOutputPacket[nNewPos] = pInputPacket[i];
		}

		return 0;
	}
	
	return -1;
}

int you_333_GeneratePacket_001(char *strCustomerName, char *strProductName, char *strMac, char *pResPacket, int *pPacketLen)
{
	//generate activation packet...
	int nLen = 0,j;

	char pOutPacket[PACKET_BYTE_NUM];
	char p5Bytes[5];

	*pPacketLen = 0;
	if(8 != strlen(strCustomerName) 
		|| 8 != strlen(strProductName)
		|| 8 != strlen(DEFAULT_CODE)) return -1;


    memset(pResPacket,0,PACKET_BYTE_NUM);

	pResPacket[0] = 'y';
	pResPacket[1] = 't';
	pResPacket[2] = '0';
	pResPacket[3] = '1';  //

	memcpy(pResPacket + 4,strCustomerName,8); //4~11
	memcpy(pResPacket + 12,strProductName,8);//12~19
	memcpy(pResPacket + 20,DEFAULT_CODE,8);//20~27

	//memcpy(pResPacket + 28,pActivationCode,8); //8 zeros: 28~35
	
	pResPacket[36] = '1';
	pResPacket[37] = '1';
	pResPacket[38] = '1';

	//connection times: 39~46: 8 bytes for connection times...
	pResPacket[39] = 0;
	pResPacket[41] = 0;
	pResPacket[43] = 0;
	pResPacket[45] = 0;

	pResPacket[40] = 'a';
	pResPacket[42] = 'z';
	pResPacket[44] = '@';
	pResPacket[46] = 'W';


	
	pResPacket[47] = '0';

	//MAC:  48~53: 6 bytes for MAC
	pResPacket[48] = strMac[0];
	pResPacket[49] = strMac[1];
	pResPacket[50] = strMac[2];
	pResPacket[51] = strMac[3];
	pResPacket[52] = strMac[4];
	pResPacket[53] = strMac[5];

	//53~96: reserved


	//encrypt this packet
	you_333_GenerateCRC(pResPacket,PACKET_BYTE_NUM,p5Bytes);
	for(j=0; j<5;j++)pResPacket[90+j] = p5Bytes[j];

	you_333_TranPacketForward(pResPacket,PACKET_BYTE_NUM,pOutPacket);
	memcpy(pResPacket,pOutPacket,PACKET_BYTE_NUM);
	*pPacketLen = PACKET_BYTE_NUM;

	return 0;
}

int you_333_GeneratePacket_002(char *pData, char save_flag, char *pResPacket, int *pPacketLen)
{
	//for normal license checking packet...
	void *fp;
	char pOutPacket[PACKET_BYTE_NUM];
	char pActivationCode[8], pMAC[6],pConnectTimes[8];
	char strCustomer[8], strProduct[8];
	char pYear[2], pMonth[1],pDate[1],pHour[1],pMinute[1],pSecond[1];
	char p5Bytes[5];

	size_t nByteRead;
	int nOffset = 0,j;

	*pPacketLen = 0;

	//byte 0~1: packet length
	nOffset = 2;

	//byte 2~9: encrypted activation code
	you_333_ModifyInfo(&pData[2],KEY_ACTIVATION_CODE,8,pActivationCode);

	//byte 10~15: encrypted MAC address
	you_333_ModifyInfo(&pData[10], KEY_MAC, 6, pMAC);

	//byte 16~23: encrypted connection times
	you_333_ModifyInfo(&pData[16], KEY_CONNECTION_TIME, 8, pConnectTimes);

	//byte 24~31: customer
	you_333_ModifyInfo(&pData[24], KEY_CUSTOMER, 8, strCustomer);

	//byte 32~39: product
	you_333_ModifyInfo(&pData[32], KEY_PRODUCT, 8, strProduct);

	//byte 40~41: year	
	you_333_ModifyInfo(&pData[40], KEY_DATE, 2, pYear);

	//byte 42: month
	you_333_ModifyInfo(&pData[42], KEY_DATE, 1, pMonth);

	//byte 43: date
	you_333_ModifyInfo(&pData[43], KEY_DATE, 1, pDate);

	//byte 44: hour
	you_333_ModifyInfo(&pData[44], KEY_DATE, 1, pHour);

	//byte 45: minute
	you_333_ModifyInfo(&pData[45], KEY_DATE, 1, pMinute);

	//byte 46: second
	you_333_ModifyInfo(&pData[46], KEY_DATE, 1, pSecond);


	//STEP 2: compose license-checking packet... 
    memset(pResPacket,0,PACKET_BYTE_NUM);

	pResPacket[0] = 'y';
	pResPacket[1] = 't';
	pResPacket[2] = '1';  //regular request for license checking
	pResPacket[3] = '1';  //
	
	memcpy(pResPacket + 4,strCustomer,8); //4~11
	memcpy(pResPacket + 12,strProduct,8);//12~19
	memcpy(pResPacket + 20,DEFAULT_CODE,8);//20~27: default code
	memcpy(pResPacket + 28,pActivationCode,8);//28~35: activation code
	pResPacket[36] = '1';
	pResPacket[37] = '1';
	pResPacket[38] = '1';

	memcpy(pResPacket + 39,pConnectTimes,8);//39~46: connection times 


	pResPacket[47] = save_flag;

	memcpy(pResPacket + 48,pMAC,6);//48~53: MAC address
	
	//54~96
	you_333_GenerateCRC(pResPacket,PACKET_BYTE_NUM,p5Bytes);
	for(j=0; j<5;j++)pResPacket[90+j] = p5Bytes[j];

	you_333_TranPacketForward(pResPacket,PACKET_BYTE_NUM,pOutPacket); //encrption before sending 
	memcpy(pResPacket,pOutPacket,PACKET_BYTE_NUM);

	*pPacketLen = PACKET_BYTE_NUM;
	return 0;
}

int you_333_KeepData_001(char *pResData, int nResDataLen, char *pInPacket, int nPacketLen)
{
	static unsigned long nRandom = 1;

	char pActivationCode[8], pMAC[6],pConnectTimes[8];
	char strCustomer[8], strProduct[8];
	char pAllDate[7];
	char pYear[2], pMonth[1],pDate[1],pHour[1],pMinute[1],pSecond[1];
	int i;
	
	if(RESDATA_BYTE_NUM != nResDataLen) return -1;

	memset(pResData, 0x00, RESDATA_BYTE_NUM);
	for(i = 128; i < RESDATA_BYTE_NUM; i++)
	{
		nRandom = nRandom * 1103515245L + 12345;
		pResData[i] = (unsigned char)(nRandom%256); 
	}

	//0~1: data length
	int nOffset = 2;
	//nOffset: 2~9: encrypt activation code

	you_333_ModifyInfo(&pInPacket[28], KEY_ACTIVATION_CODE, 8, pActivationCode);//28~35
	memcpy(&pResData[nOffset],pActivationCode,8);
	nOffset += 8;

	//nOffset: 10~15: encyrpt MAC address
	you_333_ModifyInfo(&pInPacket[48], KEY_MAC,6, pMAC);//48~53
	memcpy(&pResData[nOffset],pMAC,6);
	nOffset += 6;

	//nOffset: 16~23: connection times...
	you_333_ModifyInfo(&pInPacket[39], KEY_CONNECTION_TIME,8, pConnectTimes);//39~46
	memcpy(&pResData[nOffset],pConnectTimes,8);
	nOffset += 8;

	//nOffset: 24~31: customer name
	you_333_ModifyInfo(&pInPacket[4], KEY_CUSTOMER,8, strCustomer);//4~11
	memcpy(&pResData[nOffset],strCustomer,8);
	nOffset += 8;

	//nOffset: 32~39: product name
	you_333_ModifyInfo(&pInPacket[12], KEY_PRODUCT,8, strProduct);//12~19
	memcpy(&pResData[nOffset],strProduct,8);
	nOffset += 8;

	//nOffset: 40~46: date
	memcpy(pAllDate,&pInPacket[54],7); //54~60
	you_333_ModifyInfo(pAllDate, KEY_DATE,2, pYear);//54~55
	memcpy(&pResData[nOffset],pYear,2);
	nOffset += 2;

	you_333_ModifyInfo(&pAllDate[2], KEY_DATE,1, pMonth);//56
	memcpy(&pResData[nOffset],pMonth,1);
	nOffset ++;

	you_333_ModifyInfo(&pAllDate[3], KEY_DATE,1, pDate);//57
	memcpy(&pResData[nOffset],pDate,1);
	nOffset ++;

	you_333_ModifyInfo(&pAllDate[4], KEY_DATE,1, pHour);//58
	memcpy(&pResData[nOffset],pHour,1);
	nOffset ++;

	you_333_ModifyInfo(&pAllDate[5], KEY_DATE,1, pMinute);//59
	memcpy(&pResData[nOffset],pMinute,1);
	nOffset ++;

	you_333_ModifyInfo(&pAllDate[6], KEY_DATE,1, pSecond);//60
	memcpy(&pResData[nOffset],pSecond,1);
	nOffset ++;

	return 0;
}


int you_333_KeepPacket_001(char *pInPacket, int nInPacketLen, char *pOutPacket, int *nOutPacketLen)
{
	int i;
	char p5Bytes[5];
	
	if(PACKET_BYTE_NUM != nInPacketLen) {
		*nOutPacketLen = 0;
		return -1;
	}
	
	you_333_TranPacketBackward(pInPacket, PACKET_BYTE_NUM, pOutPacket);
	you_333_GenerateCRC(pOutPacket, PACKET_BYTE_NUM, p5Bytes);
	
	for(i=0; i < 5; i++) {
		if(pOutPacket[90+i] != p5Bytes[i]) break;
	}

	if(i != 5) {
		printf("CRC check error!!\r\n");
		*nOutPacketLen = 0;
		return -1;
	}

	if((you_333_CheckServerInfo_001(pOutPacket) < 0)//sever did NOT work normally
		|| (you_333_CheckServerInfo_001(pOutPacket) < 0) //license is NOT available
		|| (you_333_CheckServerInfo_002(pOutPacket) < 0) //license is NOT available
		|| (you_333_CheckServerInfo_003(pOutPacket) < 0 )) //license is NOT available
	{				
		*nOutPacketLen = 0;
		printf("CRC check error!!\r\n");
		return -1;
	}

	*nOutPacketLen = PACKET_BYTE_NUM;
	return 0;
}








