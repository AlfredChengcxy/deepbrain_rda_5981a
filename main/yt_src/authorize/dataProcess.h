//
//  dataProcess.h
//
//  Created by ustudy on 19/1/14.
//  Copyright ustudy. All rights reserved.
//

#ifndef __DATA_PROCESS_HEADER__
#define __DATA_PROCESS_HEADER__

#ifdef __cplusplus
 extern "C" {
#endif

#define PACKET_BYTE_NUM 97	//fixed_data_length
#define RESDATA_BYTE_NUM 256


int you_333_GeneratePacket_001(char *strCustomerName, char *strProductName, char *strMac, char *pResPacket, int *pPacketLen);
int you_333_GeneratePacket_002(char *pData,char save_flag, char *pResPacket, int *pPacketLen);

int you_333_KeepPacket_001(char *pInPacket, int nInPacketLen, char *pOutPacket, int *nOutPacketLen);
int you_333_KeepData_001(char *pResData, int nResDataLen, char *pInPacket, int nPacketLen);

int you_333_CheckServerInfo_001(char *pDecryptedServerPacket);
int you_333_CheckServerInfo_002(char *pDecryptedServerPacket);
int you_333_CheckServerInfo_003(char *pDecryptedServerPacket);
int you_333_CheckServerInfo_004(char *pDecryptedServerPacket);

#ifdef __cplusplus
 }
#endif

#endif 
