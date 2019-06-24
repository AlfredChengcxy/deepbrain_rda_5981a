#include "debug_log_interface.h"
#include "inet.h"
#include "rda5981_sniffer.h"
#include "Airkiss.h"
#include "airkissNet.h"
#include "rda_wdt_api.h"

#define LOG_TAG "airkiss"

extern nsapi_stack_t lwip_stack;

static void AirkissChangeChannel(void const *pArg);
static void AirkissWifiRX(uint8_t *buf, uint16_t len);
static void AirkissFinish();
static int AirkissHandler(unsigned short data_len, void *data);
static int AirkissSendRsp(uint8_t random);
static void AirkissScanChannel();

rtos::Semaphore gAirkissSem(0);
WiFiStackInterface * gWiFiStackInterface = NULL;
Airkiss::IOnEvent * gEvent = NULL;
rtos::RtosTimer gTimer(AirkissChangeChannel,osTimerPeriodic,0);

Airkiss Airkiss::mInstance;
int nAirkiss = 0;
airkiss_result_t result;
uint8_t uScanChannel[15];
int cur_channel = 0;
int to_ds = 1;
int from_ds = 0;
int mgm_frame = 0;

static const airkiss_config_t akconf =
{
	(airkiss_memset_fn)&memset,
	(airkiss_memcpy_fn)&memcpy,
	(airkiss_memcmp_fn)&memcmp,
	(airkiss_printf_fn)&printf
};
airkiss_context_t akcontex;


Airkiss & Airkiss::Instance()
{
	return mInstance;
}


Airkiss::Airkiss()
#if USE_THREAD	
	:thAirkiss(osPriorityNormal,(1024*1))
#endif	
{
		
}

Airkiss::~Airkiss()
{
	
}


enum AirkissMsgType {
    AIRKISS_START = 1,
    AIRKISS_FINISH
};

#if USE_THREAD	
static rtos::MemoryPool<AirkissMsgType, 2> gAirkissMsgPool;
static rtos::Queue<AirkissMsgType, 2> gAirkissMsgQueue;

void Airkiss::Run()
{
	while(1)
	{
		osEvent evt = gAirkissMsgQueue.get();
	    DEBUG_LOGI(LOG_TAG,("thread receive message, status :%d", evt.status);
	    if (evt.status == osEventMessage) 
		{
	        AirkissMsgType* sMsg = (AirkissMsgType*)evt.value.p;
			switch(*sMsg)
			{
				case AIRKISS_START:
					if(nAirkiss)
					{
						DEBUG_LOGI(LOG_TAG,"Airkissing so exit");
						AirkissStop();
						gEvent->on_airkiss_finish(NULL,NULL);
						break;
					}
					AirkissStop();
					gWiFiStackInterface->disconnect();
					rda5981_enable_sniffer(AirkissHandler);	
					airkiss_init(&akcontex, &akconf);
					gEvent->on_airkiss_wait();
					gTimer.start(200);
					nAirkiss = 1;
					break;
				case AIRKISS_FINISH:
					if(nAirkiss)
					{
						AirkissSendRsp(result.random);
						nAirkiss = 0;
					}
					break;	
			}	
			gAirkissMsgPool.free(sMsg);
		}	
	}
}
#endif

void Airkiss::SetEventListener(IOnEvent* _pEvent)
{
	gEvent = _pEvent;	
}



void Airkiss::Init(WiFiStackInterface * pWiFiStackInterface)
{
	gWiFiStackInterface = pWiFiStackInterface;	
	AirkissScanChannel();
#if USE_THREAD	
	thAirkiss.start(this,&Airkiss::Run);
#endif		
}

void Airkiss::AirkissStop()
{
	if(nAirkiss)
	{
		rda5981_stop_sniffer();
		rda5981_disable_sniffer();
		gTimer.stop();
		nAirkiss = 0;
		gAirkissSem.release();
	}
}

void Airkiss::AirkissStart()
{
	DEBUG_LOGI(LOG_TAG,"AirkissStart");
#if !USE_THREAD	
	if(nAirkiss)
	{
		DEBUG_LOGI(LOG_TAG,"Airkissing so exit");
		AirkissStop();
		gEvent->on_airkiss_finish(NULL,NULL);
		return;
	}
	AirkissStop();

	gWiFiStackInterface->disconnect();
	rda5981_enable_sniffer(AirkissHandler);	
	airkiss_init(&akcontex, &akconf);
	gEvent->on_airkiss_wait();
	gTimer.start(200);
	nAirkiss = 1;

	gAirkissSem.wait();
#else
	AirkissMsgType *sMsg = NULL;
	sMsg = gAirkissMsgPool.alloc();
	if(NULL == sMsg){DEBUG_LOGE(LOG_TAG,"gAirkissMsgPool alloc failed!");return;}
	*sMsg = AIRKISS_START;
	gAirkissMsgQueue.put(sMsg);	
#endif
}

void Airkiss::AirkissFinish()
{
#if !USE_THREAD	
	if(nAirkiss)
	{
		AirkissSendRsp(result.random);
		nAirkiss = 0;
	}
#else
	DEBUG_LOGI(LOG_TAG,"AirkissFinish");
	AirkissMsgType *sMsg = NULL;
	sMsg = gAirkissMsgPool.alloc();
	if(NULL == sMsg){DEBUG_LOGE(LOG_TAG,"gAirkissMsgPool alloc failed!");return;}
	*sMsg = AIRKISS_FINISH;
	gAirkissMsgQueue.put(sMsg);
#endif
}

bool Airkiss::IsAirkiss()
{
	return nAirkiss==1?true:false;
}

int Airkiss::AirkissScanChannelex(rda5981_scan_result * pResult,int nLen)
{
	int i;
    int ret = 0;

    ret = gWiFiStackInterface->scan(NULL, 0);
    ret = gWiFiStackInterface->scan_result(pResult, nLen);
    DEBUG_LOGI(LOG_TAG,"scan result:%d", ret);

    for(i=0; i<ret; ++i) 
	{
        if (pResult[i].channel<=13) 
		{
            DEBUG_LOGI(LOG_TAG,"i:%d,channel:%d,ssid:%s,RSSI:%d", i, pResult[i].channel,pResult[i].SSID,pResult[i].RSSI);
        }
    }
	return ret;
}


static void AirkissScanChannel()
{	
	int i;
    int ret;
    rda5981_scan_result scan_result[30];
    memset(uScanChannel, 0, sizeof(uScanChannel));
    ret = gWiFiStackInterface->scan(NULL, 0);
    ret = gWiFiStackInterface->scan_result(scan_result, 30);
    DEBUG_LOGI(LOG_TAG,"scan result:%d", ret);

    for(i=0; i<ret; ++i) 
	{
        if (scan_result[i].channel<=13) 
		{
            DEBUG_LOGI(LOG_TAG,"i:%d,channel:%d,ssid:%s,RSSI:%d", i, scan_result[i].channel,scan_result[i].SSID,scan_result[i].RSSI);
            uScanChannel[scan_result[i].channel] = 1;
        }
    }
}

static int AirkissSendRsp(uint8_t random)
{
#if 0
    int i;
    int ret;
    int udp_broadcast = 1;
    UDPSocket udp_socket;
    ip_addr_t ip4_addr;
    char host_ip_addr[16];

    memset((u8_t *)(&ip4_addr), 0xff, 4);
    strcpy(host_ip_addr, inet_ntoa(ip4_addr));

    DEBUG_LOGI(LOG_TAG,"send response to host:%s", host_ip_addr);

    ret = udp_socket.open(gWiFiStackInterface);
    if (ret) {
        DEBUG_LOGI(LOG_TAG,"open socket error:%d", ret);
        return ret;
    }
	ret = udp_socket.setsockopt(0,NSAPI_UDP_BROADCAST,&udp_broadcast,sizeof(udp_broadcast));
    if (ret) {
        DEBUG_LOGI(LOG_TAG,"setsockopt error:%d", ret);
        return ret;
    }

    for (i=0; i<30; ++i) {
        ret = udp_socket.sendto(host_ip_addr, 10000, &random, 1);
        if (ret <= 0)
            DEBUG_LOGI(LOG_TAG,"send rsp fail%d", ret);
        wait_us(100 * 1000);
    }
	udp_socket.close();
    DEBUG_LOGI(LOG_TAG,"send response to host done");
    return ret;
#else
	int ret;
	nsapi_socket_t handle;
	int udp_broadcast = 1;
	nsapi_addr_t host_addr = {NSAPI_IPv4, {255, 255, 255, 255}};

	ret = lwip_stack.stack_api->socket_open((nsapi_stack_t *)gWiFiStackInterface, &handle, NSAPI_UDP);
	if (ret) {
		DEBUG_LOGI(LOG_TAG,"open socket error: %d\r\n", ret);
		return ret;
	}
	lwip_stack.stack_api->socket_attach((nsapi_stack_t *)gWiFiStackInterface, handle, NULL, NULL);
	lwip_stack.stack_api->setsockopt((nsapi_stack_t *)gWiFiStackInterface, handle, 0,NSAPI_UDP_BROADCAST, &udp_broadcast, sizeof(udp_broadcast));

	for (int i = 0; i < 30; i++) 
	{
		ret = lwip_stack.stack_api->socket_sendto((nsapi_stack_t *)gWiFiStackInterface, handle,host_addr, 10000, (const void *) &(random), 1);

		if (ret <= 0) 
		{
			DEBUG_LOGI(LOG_TAG,"udp socket send ret: %d\r\n", ret);
		}
		wait_us(100 * 1000);
	}
	lwip_stack.stack_api->socket_close((nsapi_stack_t *)gWiFiStackInterface, handle);
	DEBUG_LOGI(LOG_TAG,"send response to host done");
	return ret;
#endif
}

static void AirkissChangeChannel(void const *pArg)
{	
    int i;
    do {
        cur_channel++;
    	if (cur_channel > 13) {
    		cur_channel = 1;
            to_ds = to_ds?0:1;
            from_ds = from_ds?0:1;
    	}
    } while (uScanChannel[cur_channel] == 0);

	DEBUG_LOGI(LOG_TAG,"cur_channel = %d",cur_channel);
    rda5981_start_sniffer(cur_channel, to_ds, from_ds, mgm_frame, 0);
    airkiss_change_channel(&akcontex);
}


static void AirkissFinish()
{
	int err;
	
	err = airkiss_get_result(&akcontex, &result);
	if (err == 0)
	{
		DEBUG_LOGI(LOG_TAG,"airkiss_get_result() ok!");
        DEBUG_LOGI(LOG_TAG,"ssid = \"%s\", pwd = \"%s\", ssid_length = %d, \"pwd_length = %d, random = 0x%02x",
        result.ssid, result.pwd, result.ssid_length, result.pwd_length, result.random);
        rda5981_stop_sniffer();
       	//rda5981_flash_write_sta_data(result.ssid, result.pwd);
       	
		
		gEvent->on_airkiss_finish(result.ssid, result.pwd);
	}
	else
	{
		DEBUG_LOGI(LOG_TAG,"airkiss_get_result() failed !");
	}

	gAirkissSem.release();
}


static void AirkissWifiRX(uint8_t *buf, uint16_t len)
{
	int ret;
	DEBUG_LOGD(LOG_TAG,"len = %d",len);
	ret = airkiss_recv(&akcontex, buf, len);
	if ( ret == AIRKISS_STATUS_CHANNEL_LOCKED)
	{
		gEvent->on_airkiss_connect();
		DEBUG_LOGW(LOG_TAG,"AIRKISS_STATUS_CHANNEL_LOCKED");
		gTimer.stop();				
	}    
	else if ( ret == AIRKISS_STATUS_COMPLETE )
	{
		
		DEBUG_LOGI(LOG_TAG,"AIRKISS_STATUS_COMPLETE");
		AirkissFinish();
	}
}

static int AirkissHandler(unsigned short data_len, void *data)
{
    static unsigned short seq_num = 0, seq_tmp;
    char *frame = (char *)data;
    seq_tmp = (frame[22] | (frame[23]<<8))>>4;
	
    if (seq_tmp == seq_num) return 0;   
    else seq_num = seq_tmp;
	
    AirkissWifiRX((uint8_t*)data, data_len);
    return 0;
}






