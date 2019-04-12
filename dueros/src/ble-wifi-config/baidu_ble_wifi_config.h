// Copyright 2018 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: get ssid and password from ble

#ifndef BAIDU_TINYDU_IOT_OS_SRC_BAIDU_BLE_WIFI_CONFIG_H
#define BAIDU_TINYDU_IOT_OS_SRC_BAIDU_BLE_WIFI_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Get wifi ssid and password from ble
 * must stop play and record, before call this function
 *
 * @Param ssid, output buffer of ssid
 * @Param password, output buffer of password
 * @Return int, success: 0, fail: -1
 */
int duer_ble_get_ssid_and_password(char ssid[32], char password[64]);

/*
 * Send response to host, if duer_ble_get_ssid_and_password has been called,
 * must call this function to exit bt mode
 *
 * @Param ipaddr, if connect wifi successfully, pass ip address, otherwise NULL
 * @Return int, success: 0, fail: -1
 */
int duer_ble_send_response_to_host(const char *ipaddr);

/*
 * Is conifing the wifi by ble now
 *
 * @Return int, yes: 1, no: 0
 */
int duer_ble_is_configing_wifi();

#ifdef __cplusplus
}
#endif

#endif // BAIDU_TINYDU_IOT_OS_SRC_BAIDU_BLE_WIFI_CONFIG_H
