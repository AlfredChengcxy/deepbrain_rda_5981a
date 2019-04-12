#ifndef __YT_VBAT_H__
#define __YT_VBAT_H__

enum VBAT_SCENE
{
    VBAT_FULL,
    VBAT_NORMAL,
    VBAT_LOW_POWER,
    VBAT_VERY_LOW_POWER,
    VBAT_SHUTDOWN
};

bool vbat_is_shutdown();

void vbat_start();
bool vbat_check_startup();

#endif

