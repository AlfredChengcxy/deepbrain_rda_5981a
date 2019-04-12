#ifndef BLECONFIG_H
#define BLECONFIG_H

#include "rda58xx.h"

class BleConfig
{
public:
    bool isReady(void);
    void setUartMode(void);
    void setBtMode(void);
    void switchToBleMode(void);
    void startBleConfigService(void);
    rda58xx_mode getMode(void);
    void setBtAddr(const unsigned char addr[12]);
    void setBtName(const unsigned char* name);
    void leSetIndication(const char* state);
    void leSetFeature(const char* state);
    int leGetConfigState(void);
    int leGetIndState(void);
    void leGetConfigInfo(char *configInfo);
    int leClearConfigInfo(void);

private:

};

#endif
