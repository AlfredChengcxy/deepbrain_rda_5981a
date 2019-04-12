// Copyright 2018 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: get ssid and password from ble

#include "baidu_ble_wifi_config.h"
#include "mbed.h"
#include "bleconfig.h"
#include "WiFiStackInterface.h"
#include "lightduer_lib.h"
#include "lightduer_log.h"

static const int CONFIG_INFO_LENGTH = 128;
static const int SSID_LENGTH = 32;
static const int PASSWORD_LENGTH = 64;
static const char *BLE_FEATURE = "778899001122";
static const int NORMAL_SLEEP_TIME = 500;       // unit:ms
static const int SWITCH_BT_SLEEP_TIME = 1000;   // unit:ms
static volatile int s_is_configing_wifi = 0;

extern void* get_network_interface(void);

int duer_ble_get_ssid_and_password(char ssid[32], char password[64])
{
    BleConfig config;
    char config_info[CONFIG_INFO_LENGTH];

    if (ssid == NULL || password == NULL) {
        DUER_LOGI("invalid arguments");
        return -1;
    }

    while (!config.isReady()) {
        DUER_LOGD("Codec Not Ready");
        Thread::wait(NORMAL_SLEEP_TIME);
    }

    WiFiStackInterface* wifi = (WiFiStackInterface*)get_network_interface();
    if (wifi == NULL) {
        DUER_LOGE("WiFiStackInterface is NULL");
        return -1;
    }

    const char *mac_addr = wifi->get_mac_address();
    if (mac_addr == NULL) {
        DUER_LOGE("mac address is NULL");
        return -1;
    }

    s_is_configing_wifi = 1;

    const int ADDR_LEN = 13;
    unsigned char addr[ADDR_LEN];
    int i = 0;
    while (*mac_addr && i < ADDR_LEN) {
        if (*mac_addr != ':') {
            addr[i] = *mac_addr;
            i++;
        }
        mac_addr++;
    }
    addr[ADDR_LEN - 1] = 0;
    DUER_LOGI("setBtAddr: %s", addr);

    config.startBleConfigService();
    config.setBtAddr(addr);

    config.setBtMode();
    while (config.getMode() != BT_MODE) {
        Thread::wait(NORMAL_SLEEP_TIME);
    }
    Thread::wait(SWITCH_BT_SLEEP_TIME);

    config.switchToBleMode();

    config.leSetFeature(BLE_FEATURE);

    memset(config_info, 0, sizeof(config_info));
    memset(ssid, 0, SSID_LENGTH);
    memset(password, 0, PASSWORD_LENGTH);

    while (config_info[0] == 0) {
        while (config.leGetConfigState() < 1) {
            Thread::wait(NORMAL_SLEEP_TIME);
        }

        config.leGetConfigInfo(config_info);
        config.leClearConfigInfo();
        DUER_LOGI("config_info: %s.", config_info);
    }

    int ret = DUER_SSCANF(config_info, "%s\n%s\n", ssid, password);
    if (ret != 2) {
        DUER_LOGE("config_info invalid format");
        return -1;
    }

    return 0;
}

int duer_ble_send_response_to_host(const char *ipadr)
{
    if (s_is_configing_wifi) {
        BleConfig config;
        if (ipadr) {
            config.leSetIndication(ipadr);
        } else {
            config.leSetIndication("failed");
        }
        Thread::wait(NORMAL_SLEEP_TIME);
        config.setUartMode();
        s_is_configing_wifi = 0;
    }

    return 0;
}

int duer_ble_is_configing_wifi()
{
    return s_is_configing_wifi;
}
