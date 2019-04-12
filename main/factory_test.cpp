#if 1
#include "mbed.h"
#include "rtos.h"
#include "console.h"
#include "WiFiStackInterface.h"
#include "rda_wdt_api.h"
#include "rda_ccfg_api.h"
#include "cmsis_os.h"
#include "rda_wdt_api.h"
#include "YTManage.h"
#include "rda58xx.h"
#include "SDMMCFileSystem.h"
#include "events.h"
#include "lwip_stack.h"
#include "yt_key.h"
#include "cJson.h"

static char *version = "**********RDA Software Version sta_V1.0_110**********";
static int op_mode = 0;//initial ~ 0 factory test ~ 1 function ~ 2
static char conn_flag = 0;
static WiFiStackInterface wifi;
extern unsigned char rda_mac_addr[6];
extern rda58xx _rda58xx;
extern SDMMCFileSystem sd;

int do_factory_test_5856(cmd_tbl_t* cmd, int argc, char* argv[]);

enum AUTO_TEST_MODE
{
	AT_UART_MODE,
	AT_SD_MODE
};

AUTO_TEST_MODE atmMode = AT_UART_MODE;





int do_conn_state( cmd_tbl_t *cmd, int argc, char *argv[])
{
    if(op_mode != 1)
        return 0;
    
    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }
#if 1
    const char *ssid;
    ssid = wifi.get_ssid();
    if(ssid[0])
        printf("CONNECTED! ssid:%s RSSI:%d db ip:%s\r\n", wifi.get_ssid(), wifi.get_rssi(), wifi.get_ip_address());
    else
        printf("NOT CONNECTED!\r\n");
#else
    if(wifi.bss.SSID[0])
        printf("CONNECTED! ssid:%s RSSI:%d db\r\n", wifi.bss.SSID, wifi.bss.SSID);
    else
        printf("NOT CONNECTED!\r\n");
#endif
    return 0;
}

int do_get_mac( cmd_tbl_t *cmd, int argc, char *argv[])
{
    if(op_mode != 1)
        return 0;
    
    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }
    
    char mac[6];
    mbed_mac_address(mac);
    printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			
	return 0;
}

int do_set_mac( cmd_tbl_t *cmd, int argc, char *argv[])
{
    char *mdata, mac[6], i;

    if(op_mode != 1)
        return 0;
    
    if (argc < 2) {
        show_cmd_usage(cmd);
        return -1;
    }
    
    if(strlen(argv[1]) != 12){
        printf("Error MAC address len\r\n");
        return;
    }
    mdata = argv[1];
    for(i = 0; i < 12; i++){
        if(mdata[i] >= 0x41 && mdata[i] <= 0x46)// switch 'A' to 'a'
            mdata[i] += 0x20;
        if(mdata[i] >= 0x61 && mdata[i] <= 0x66)//switch "ab" to 0xab
            mac[i] = mdata[i] - 0x57;
        if(mdata[i] >= 0x30 && mdata[i] <= 0x39)
                mac[i] = mdata[i] - 0x30;
            if(i%2 == 1)
                mac[i/2] = mac[i-1] << 4 | mac[i];
    }
    if(!mac_is_valid(mac)){
        printf("MAC is ZERO\r\n");
        return 0;
    }
    memcpy(rda_mac_addr, mac, 6);
    rda5981_flash_write_mac_addr(rda_mac_addr);
    return 0;
}

int do_get_ver( cmd_tbl_t *cmd, int argc, char *argv[])
{
    if(op_mode != 1)
        return 0;
    
    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }
    printf("Software Version: %s\r\n", version);
    return 0;
}

int do_write_usedata( cmd_tbl_t *cmd, int argc, char *argv[])
{

    int len, ret, tmp_len = 0;
    unsigned char *buf;
	unsigned char local_buf[64] = {0};
    if(op_mode != 1)
        return 0;
    
    if (argc < 2) {
        show_cmd_usage(cmd);
        return -1;
    }

    len = atoi(argv[1]);
    buf = (unsigned char *)malloc(++len);
    memset(buf, 0, len);
    
    do{
        unsigned int size;
        size = console_fifo_get(local_buf, len-tmp_len);
        if(size > 0){
            memcpy(&buf[tmp_len], local_buf, size);
            tmp_len += size;
        }
    }while(tmp_len < len);
    //printf("write data %s\r\n", buf);
    ret = rda5981_flash_write_3rdparter_data(&buf[1], len-1);
    if(ret < 0)
        printf("write flash error, error %d\r\n", ret);
    else
        printf("Data write complete\r\n");
    free(buf);
    return 0;
}

int do_read_usedata( cmd_tbl_t *cmd, int argc, char *argv[])
{

    int len, ret, tmp_len = 0;
    unsigned char *buf;
    
    if(op_mode != 1)
        return 0;
    
    if (argc < 2) {
        show_cmd_usage(cmd);
        return -1;
    }

    len = atoi(argv[1]);
    buf = (unsigned char *)malloc(len+3);
    memset(buf, 0, len+1);

    ret = rda5981_flash_read_3rdparter_data(buf, len);
    if(ret < 0)
        printf("read flash error, error %d\r\n", ret);
    else
        printf("Data read complete\r\n");
    buf[len] = 0x0D;
	buf[len+1] = 0x0A;
	buf[len+2] = 0x0;
    console_puts((char *)buf);
    free(buf);
    return 0;
}

int do_reset( cmd_tbl_t *cmd, int argc, char *argv[])
{
    if(op_mode != 1)
        return 0;
    
    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }
    printf("SOFTWARE RESET!!!!\r\n");
	rda_wdt_softreset();
    return 0;

}

int do_conn( cmd_tbl_t *cmd, int argc, char *argv[])
{
    char *ssid_t, *pw_t;
    const char *ip_addr;
	char ssid[20];
	char pw[20];

    if(op_mode != 1)
        return 0;
    
    if(conn_flag == 1){
        printf("error! Has been connected!");
        return;
    }
    printf("OK, start connect\r\n");

    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }

    memset(ssid, 0, sizeof(ssid));
    memset(pw, 0, sizeof(pw));
	
	if(argc > 1)
    	memcpy(ssid, argv[1], strlen(argv[1]));

    if(argc > 2)
        memcpy(pw, argv[2], strlen(argv[2]));
    
    if(strlen(ssid) != 0)
        ssid_t = ssid;
    else
        ssid_t = NULL;
    
    if(strlen(pw) != 0)
        pw_t = pw;
    else
        pw_t = NULL;

    printf("ssid %s pw %s\r\n", ssid_t, pw_t);
    
    int ret = 0;
    int reconn=0, scan_times=0, find = 0;
    int scan_res = 0;
	const int SCAN_TIMES = 10;
    rda5981_scan_result *bss_list = NULL;

    mbed_lwip_init(NULL);

	rda5981_del_scan_all_result();
	while (scan_times++ < SCAN_TIMES) {
		scan_res = rda5981_scan(ssid_t, strlen(ssid_t), 0);
		find = rda5981_check_scan_result(ssid, NULL, 0);
		if(find == 0) {
			printf("WiFi Factory : Succeed\r\n");
		    return 0;	   
		}
	}
	printf("WiFi Factory : Fail\r\n");
	
    //rda5981_flash_write_smartconfig_data();
    return 0;
}

int do_disconn( cmd_tbl_t *cmd, int argc, char *argv[])
{
    const char *ip;

    if(op_mode != 1)
        return 0;

    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }

    if(conn_flag == 0){
        printf("Not connectted!\r\n");
        return;
    }

    wifi.disconnect();
    while(wifi.get_ip_address() != NULL)
        Thread::wait(20);
    conn_flag = 0;
    //printf("OK, Disconnect successful\r\n");
    return 0;
}

int do_set_baud( cmd_tbl_t *cmd, int argc, char *argv[])
{
    unsigned int baudrate;

    if(op_mode != 1)
        return 0;

    if (argc < 2) {
        show_cmd_usage(cmd);
        return -1;
    }
    
    printf("OK, do set baud\r\n");
    baudrate = atoi(argv[1]);
    console_set_baudrate(baudrate);

    return 0;
}

void add_cmd()
{
    int i;
    cmd_tbl_t cmd_list[] = {
        {
            "reset",      1,   do_reset,
            "reset        - Software Reset\n"
        },
        {
            "conn",       3,   do_conn,
            "conn         - start connect\n"
        },
        {
            "setbaud",    2,   do_set_baud,
            "setbaud      - set serial baudrate\n"
        },
        {
            "connstate",  1,   do_conn_state,
            "connstate    - get conn state\n"
        },
        {
            "setmac",     2,   do_set_mac,
            "setmac       - set mac address\n"
        },
        {
            "getmac",     1,   do_get_mac,
            "getmac       - get mac address\n"
        },		
        {
            "getver",     1,   do_get_ver,
            "getver       - get ver address\n"
        },
        {
            "disconn",    1,   do_disconn,
            "disconn      - start disconnect\n"
        },
        {
            "wuserdata",  2,   do_write_usedata,
            "wusedate     - write user data\n"
        },
        {
            "ruserdata",  2,   do_read_usedata,
            "ruserdate     - read user data\n"
        },
    };
    i = sizeof(cmd_list)/sizeof(cmd_tbl_t);
    while(i--){
        if(0 != console_cmd_add(&cmd_list[i])) {
            printf("Add cmd failed\r\n");
        }
    }
}

int factory_test()
{
	unsigned int flag = (*((volatile unsigned int *)(0x40001024UL)));
	printf("flag = %x\r\n", flag);
	if((flag & (0x01UL << 29)) == 0)
		return 0;
	
	op_mode = 1;
	console_init();
    add_cmd();
	
	while(1);
	return 0;
}


static Thread s_auto_test_thread(osPriorityNormal, 4096);

static void yt_auto_test_wifi()
{
	int ret;	
	char ssid[32] = {0};
	char passwd[64] = {0};
	char ch = 0;
	int i = 0;
    printf("yt_test_wifi\r\n");

	FILE *fp = fopen("/sd/test/wifi.txt", "r");
	if(NULL == fp) {
		printf("Could not open file for write!\r\n");
		return;
	}

#if 0
	duer::YTMediaManager::instance().play_local("/sd/test/3.mp3",NULL);
	while(duer::MediaManager::instance().get_media_player_status() == duer::MEDIA_PLAYER_PLAYING)	
	{
		wait_ms(50);
	}
#endif
	while(feof(fp) == 0)
	{
		ch = fgetc(fp);
			
		if((ch != '#') && (i < 32))
		{
			ssid[i] = ch;
			i++;
		}
		else
		{
			break;
		}
	}

	i = 0;

	while(feof(fp) == 0)
	{
		ch = fgetc(fp);
		if((ch != 0)&& (ch != '#') && (i < 64))
		{
			passwd[i] = ch;
			i++;
		}
		else
		{
			break;
		}
	}
	printf("ssid: %s psd: %s\r\n",ssid,passwd);
	
    if(wifi.connect(ssid, passwd, NULL, NSAPI_SECURITY_NONE) == 0)
	{
		printf("yt_test_wifi: Succeed\r\n");
		
		//duer::YTMediaManager::instance().play_local("/sd/test/4.mp3",NULL);
		duer::YTMediaManager::instance().play_local("/sd/test/net_pass.mp3",NULL);
		
		while(duer::MediaManager::instance().get_media_player_status() == duer::MEDIA_PLAYER_PLAYING)	
		{
			wait_ms(50);
		}
	}
	else
	{	
		printf("yt_test_wifi: Fail\r\n");
		
		//duer::YTMediaManager::instance().play_local("/sd/test/7.mp3",NULL);
	duer::YTMediaManager::instance().play_local("/sd/test/net_not_pass.mp3",NULL);
		
		while(duer::MediaManager::instance().get_media_player_status() == duer::MEDIA_PLAYER_PLAYING)	
		{
			wait_ms(50);
		}
	}
}



static int on_start_record() 
{
    printf("on_start_record begin\r\n");

    rda58xx_at_status ret = _rda58xx.stopPlay(WITHOUT_ENDING);
    _rda58xx.atHandler(ret);

	ret = _rda58xx.startRecord(MCI_TYPE_AMR, 640, 8000);

	_rda58xx.atHandler(ret);
    _rda58xx.setStatus(RECORDING);

    return 0;
}

static int on_stop_record() 
{
    printf("on_stop_record begin\r\n");
	
    rda58xx_at_status ret = _rda58xx.stopRecord();
    _rda58xx.atHandler(ret);
    _rda58xx.clearBufferStatus();
    return 0;
}

static size_t on_read(void* data, size_t size) 
{
    unsigned int len = 0;

    if (FULL == _rda58xx.getBufferStatus()) {
        _rda58xx.clearBufferStatus();
        len = _rda58xx.getBufferSize();
    }

    if (len > size) {
        len = size;
    }

    if (len > 0) {
        memcpy(data, _rda58xx.getBufferAddr(), len);
    }

    return len;
}


const char amr_head[] =  "#!AMR\n";

static void yt_auto_test_5856()
{	
	int totle = 0;
	char *data = new char[8 * 1024];
	int len = 0;
	FILE *fp = fopen("/sd/test/record.amr", "w+");
	if(NULL == fp) {
		printf("Could not open file for write!\r\n");
		return;
	}
	
	duer::YTMediaManager::instance().play_local(
		"/sd/test/5.mp3",NULL);
	while(duer::MediaManager::instance().get_media_player_status() == duer::MEDIA_PLAYER_PLAYING)	
	{
		wait_ms(50);
	}


	fwrite(amr_head, strlen(amr_head), 1, fp);

	on_start_record();

	while(1)
	{
		len = on_read((void*)(data + totle), 640);		
		totle += len;
		if(totle > 5 * 1024)
		{				
			break;
		}
	}
	
	on_stop_record();
	//duer::YTMediaManager::instance().play_data(YT_AUDIO_SUCCESS,sizeof(YT_AUDIO_SUCCESS),NULL);
	
	fwrite(data, totle, 1, fp);
	fclose(fp);
	delete [] data;
	
	while(duer::MediaManager::instance().get_media_player_status() == duer::MEDIA_PLAYER_PLAYING)	
	{
		wait_ms(50);
	}

	duer::YTMediaManager::instance().play_local("/sd/test/record.amr"
		,NULL);

	while(duer::MediaManager::instance().get_media_player_status() == duer::MEDIA_PLAYER_PLAYING)	
	{
		wait_ms(50);
	}
}

static int yt_auto_test_sd()
{
	int ret = sd.disk_initialize();
	if(ret == 0)
	{
		duer::YTMediaManager::instance().play_local("/sd/test/2.mp3"
			,NULL);
		
		while(duer::MediaManager::instance().get_media_player_status() == duer::MEDIA_PLAYER_PLAYING)	
		{
			wait_ms(50);
		}
		
		return 0;			
	}
	else
	{		
		duer::YTMediaManager::instance().play_local("/sd/test/1.mp3"
			,NULL);
		while(duer::MediaManager::instance().get_media_player_status() == duer::MEDIA_PLAYER_PLAYING)	
		{
			wait_ms(50);
		}
		return -1;			
	}
}


int nKeyCount = 0;

static void yt_auto_test_key()
{
	nKeyCount |= 0x000001;
	printf("key0_fall_handle\r\n");
	if(atmMode == AT_SD_MODE)
	duer::YTMediaManager::instance().play_local("/sd/test/6.mp3",NULL);
}
static void yt_auto_test_key1()
{
	nKeyCount |= 0x000010;
	printf("key1_fall_handle\r\n");
	if(atmMode == AT_SD_MODE)
	duer::YTMediaManager::instance().play_local("/sd/test/61.mp3",NULL);
}
static void yt_auto_test_key2()
{
	nKeyCount |= 0x000100;
	printf("key2_fall_handle\r\n");
	if(atmMode == AT_SD_MODE)
	duer::YTMediaManager::instance().play_local("/sd/test/62.mp3",NULL);
}
static void yt_auto_test_key3()
{
	nKeyCount |= 0x001000;
	printf("key3_fall_handle\r\n");
	if(atmMode == AT_SD_MODE)
	duer::YTMediaManager::instance().play_local("/sd/test/63.mp3",NULL);
}
static void yt_auto_test_key4()
{
	nKeyCount |= 0x010000;
	printf("key4_fall_handle\r\n");
	if(atmMode == AT_SD_MODE)
	duer::YTMediaManager::instance().play_local("/sd/test/64.mp3",NULL);
}

static void yt_auto_test_key5()
{
	nKeyCount |= 0x100000;
	printf("key5_fall_handle\r\n");
	if(atmMode == AT_SD_MODE)
	duer::YTMediaManager::instance().play_local("/sd/test/65.mp3",NULL);
}


static void key_init()
{
	duer::event_set_handler(duer::EVT_KEY_FACTORY_KEY, yt_auto_test_key);	
	duer::event_set_handler(duer::EVT_KEY_FACTORY1_KEY, yt_auto_test_key1);
	duer::event_set_handler(duer::EVT_KEY_FACTORY2_KEY, yt_auto_test_key2);
	duer::event_set_handler(duer::EVT_KEY_FACTORY3_KEY, yt_auto_test_key3);
	duer::event_set_handler(duer::EVT_KEY_FACTORY4_KEY, yt_auto_test_key4);
	duer::event_set_handler(duer::EVT_KEY_FACTORY5_KEY, yt_auto_test_key5);
}

static void yt_auto_test_main()
{

	if(atmMode == AT_SD_MODE)
	{
	
	#if 0
		if(yt_auto_test_sd() < 0)
			return;
	#endif
	
		yt_auto_test_wifi();
		

		printf("yt_auto_test_main regist key\r\n");
		duer::event_set_handler(duer::EVT_KEY_FACTORY_KEY, yt_auto_test_key);	
		duer::event_set_handler(duer::EVT_KEY_FACTORY1_KEY, yt_auto_test_key1);
		duer::event_set_handler(duer::EVT_KEY_FACTORY2_KEY, yt_auto_test_key2);
		duer::event_set_handler(duer::EVT_KEY_FACTORY3_KEY, yt_auto_test_key3);
		duer::event_set_handler(duer::EVT_KEY_FACTORY4_KEY, yt_auto_test_key4);
		duer::event_set_handler(duer::EVT_KEY_FACTORY5_KEY, yt_auto_test_key5);
		nKeyCount = 0;
			
		while(nKeyCount != 0x001111)///需要做修改
		{
			wait_ms(50);
		}
		
	#if 0	
		yt_auto_test_5856();	
	#else
		do_factory_test_5856(NULL,1,NULL);
	#endif
	}



}

void yt_auto_test_start()
{
	s_auto_test_thread.start(yt_auto_test_main);
}




/////////////////////////////////////////////////////////////////////////////////////
int do_get_ver_5856(cmd_tbl_t* cmd, int argc, char* argv[]) {
    char ver[32] = {0};
    rda58xx_at_status ret;

    if (op_mode != 1) {
        return 0;
    }

    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }

    printf("do_get_ver_5856\r\n");

    while (!_rda58xx.isPowerOn()) {
        Thread::wait(100);
    }

    ret = _rda58xx.getChipVersion(ver);

    if (VACK == ret) {
        printf("5856 Software Version: %s\r\n", ver);
        return 0;
    } else {
        printf("5856 Software Version: Fail!\r\n");
        return -1;
    }
}
int do_factory_test_5856(cmd_tbl_t* cmd, int argc, char* argv[]) {
    rda58xx_at_status ret;

    if (op_mode != 1) {
		printf("op_mode != 1\r\n");
        return 0;
    }

    if (argc < 1) {
		printf("argc < 1\r\n");
        show_cmd_usage(cmd);
        return -1;
    }

    printf("do_factory_test_5856\r\n");
	//speakenable = 1;
    ret = _rda58xx.factoryTest(FT_ENABLE);

    if (VACK == ret) {
        printf("5856 Factory Test : Succeed\r\n");
		if(atmMode == AT_SD_MODE)
		{
			///after FT_ENABLE can't play 		
			//duer::YTMediaManager::instance().play_local("/sd/test/mic_pass.mp3",NULL);
			//while(duer::MediaManager::instance().get_media_player_status() == duer::MEDIA_PLAYER_PLAYING)	
			//{
			//	wait_ms(50);
			//}
		}
        return 0;
    } else {
        printf("5856 Factory Test : Fail\r\n");
		if(atmMode == AT_SD_MODE)
		{			
			duer::YTMediaManager::instance().play_local("/sd/test/mic_not_pass.mp3",NULL);
			while(duer::MediaManager::instance().get_media_player_status() == duer::MEDIA_PLAYER_PLAYING)	
			{
				wait_ms(50);
			}
		}		
        return -1;
    }
}
int gen_qr_code(cmd_tbl_t* cmd, int argc, char* argv[])
{
	if (op_mode != 1) {
        return 0;
    }
#if 0	
	char pQrCode[256]={0};
	int iRet = GenQrCode(pQrCode);
	if(!iRet)printf("profileStart%sprofileEnd\r\n",pQrCode);
	return iRet;
#else
	return 0;
#endif
}


int do_get_keys(cmd_tbl_t* cmd, int argc, char* argv[])
{    
	if (op_mode != 1) 
	{        
		return 0;    
	}    
	if (argc < 1) 
	{        
		show_cmd_usage(cmd);
		return -1;    
	}		
	printf("keys_start");	

#if 0	
	cJSON *root = NULL;	
	root = cJSON_CreateObject();
	if(!root)return -1;
	cJSON_AddNumberToObject(root, "key_number", 4);		
	//cJSON_AddStringToObject(root, "key0_fall_handle", "magic_key");	
	//cJSON_AddStringToObject(root, "key1_fall_handle", "wechat_key");	
	//cJSON_AddStringToObject(root, "key2_fall_handle", "pig_key");	
	//cJSON_AddStringToObject(root, "key3_fall_handle", "talk_key");	

	
	char *strJson = cJSON_Print(root);	

	printf("%s",strJson);	
	cJSON_Delete(root); 
	free(strJson);
#endif		
	printf("key_number%d\r\n",4);
	printf("keys_end\r\n");	
	
#if 1
	key_init();
#endif	

		
}

int do_key_test(cmd_tbl_t* cmd, int argc, char* argv[])
{
    if (op_mode != 1) {
        return 0;
    }
    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }
#if 0
	key_init();
#endif
    return 0;
}

void add_cmd1() {
    int i;
    cmd_tbl_t cmd_list[] = {
        {
            "reset",      1,   do_reset,
            "reset        - Software Reset\n"
        },
        {
            "conn",       3,   do_conn,
            "conn         - start connect\n"
        },
        {
            "setbaud",    2,   do_set_baud,
            "setbaud      - set serial baudrate\n"
        },
        {
            "connstate",  1,   do_conn_state,
            "connstate    - get conn state\n"
        },
        {
            "setmac",     2,   do_set_mac,
            "setmac       - set mac address\n"
        },
        {
            "getmac",     1,   do_get_mac,
            "getmac       - get mac address\n"
        },
        {
            "getver",     1,   do_get_ver,
            "getver       - get version\n"
        },
        {
            "getver5856", 1,   do_get_ver_5856,
            "getver5856   - get 5856 version\n"
        },
        {
            "ftest5856",  1,   do_factory_test_5856,
            "ftest5856    - 5856 factory test\n"
        },
    #if 0 
		{
			"ftest5670",  1,   do_factory_test_5670,
			"ftest5670	  - 5670 factory test\n"
		},
	#endif	
//		{
//			"ftestsd",  1,   do_sd_test,
//			"ftestsd	  - sd factory test\n"
//		},

		{
			"getkeys", 1,	 do_get_keys,
			"getkeys	  - key factory test\n"
		},

		{
			"ftestkey",	1,	 do_key_test,
			"ftestkey	  - key factory test\n"
		},
        {
            "disconn",    1,   do_disconn,
            "disconn      - start disconnect\n"
        },
        {
            "wuserdata",  2,   do_write_usedata,
            "wusedate     - write user data\n"
        },
        {
            "ruserdata",  1,   do_read_usedata,
            "ruserdate     - read user data\n"
        },
        {
        	"qrcode", 1 ,gen_qr_code,
			"qrcode		- gen qr code\n"
        },
    };
    i = sizeof(cmd_list) / sizeof(cmd_tbl_t);

    while (i--) {
        if (0 != console_cmd_add(&cmd_list[i])) {
            printf("Add cmd failed\r\n");
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////

int yt_auto_test()
{

////add   优先进行串口测试(适用于夹具测试)
URAT_CHECK:


#if 1
	atmMode = AT_UART_MODE;
	unsigned int flag = (*((volatile unsigned int*)(0x40001024UL)));
    //printf("flag = %x\r\n", flag);
    if ((flag & (0x01UL << 29)) == 0) {
		printf("goto DISK_CHECK\r\n");
		op_mode = 1;
        goto DISK_CHECK;
    }
	op_mode = 1;
    console_init();
	printf("Enter_Factory_Mode\r\n");
    add_cmd1();
	do_get_keys(NULL,1,NULL);
	return 1;
#endif

	
////add

DISK_CHECK:// 无串口测试的话 进行 T 卡测试(适用于整机测试)

	if(sd.disk_status() != 0)
	{
		printf("sd.disk_status() != 0\r\n");
		return 0;
	}

	FILE *fp = fopen("/sd/test/youngtone_need_factory_test.txt", "r");
	if(NULL == fp) {
		printf("NULL == fp\r\n");
		return 0;
	}	

	atmMode = AT_SD_MODE;
	return 1;
}


#endif