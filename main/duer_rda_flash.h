// Copyright (2018) Baidu Inc. All rights reserved.
/**
 * File: duer_rda_flash.c
 * Auth: Sijun Li(lisijun@baidu.com)
 * Desc: Adapter of RDA APIs for flash read/write/erase.
 */

#ifndef BAIDU_IOT_TINYDU_DEMO_DUER_RDA_FLASH_H
#define BAIDU_IOT_TINYDU_DEMO_DUER_RDA_FLASH_H

#include "rda5991h_wland.h"

// Initialize RDA FLASH
#define RDA_FLASH_SIZE                  0x00100000   // Flash Size
#define RDA_SYS_DATA_ADDR               0x180F6000 // System Data Area, fixed size 4KB
#define RDA_USER_DATA_ADDR              0x180F7000 // User Data Area start address
#define RDA_USER_DATA_LENG              0x3000     // User Data Area Length

#endif //BAIDU_IOT_TINYDU_DEMO_DUER_RDA_FLASH_H
