#include "bleconfig.h"
#include "rda58xx.h"

extern rda58xx _rda58xx;

/*  This function checks if the codec is ready  */
bool BleConfig::isReady(void)
{
    return _rda58xx.isReady();
}

void BleConfig::setUartMode(void)
{
    rda58xx_at_status ret;
    ret = _rda58xx.setMode(UART_MODE);
    _rda58xx.atHandler(ret);
}

void BleConfig::setBtMode(void)
{
    rda58xx_at_status ret;
    ret = _rda58xx.setMode(BT_MODE);
    _rda58xx.atHandler(ret);
}

void BleConfig::switchToBleMode(void)
{
    rda58xx_at_status ret;
    ret = _rda58xx.setBtLeMode();
    _rda58xx.atHandler(ret);
}

void BleConfig::startBleConfigService(void)
{
	unsigned char adv[31]={0x09, 0x09, 0x77, 0x69,0x66, 0x69, 0x5f, 0x73, 0x63, 0x73, 0x02, 0x01, 0x06};
    rda58xx_at_status ret;
    ret = _rda58xx.lesWifiScs(adv);
    _rda58xx.atHandler(ret);
}

void BleConfig::setBtAddr(const unsigned char addr[12])
{
    rda58xx_at_status ret;
    ret = _rda58xx.SetAddr(addr);
    _rda58xx.atHandler(ret);
}

void BleConfig::setBtName(const unsigned char* name )
{
    rda58xx_at_status ret;
    ret = _rda58xx.SetName(name);
    _rda58xx.atHandler(ret);
}

rda58xx_mode BleConfig::getMode(void)
{
    return _rda58xx.getMode();
}

void BleConfig::leGetConfigInfo(char *configInfo)
{
    rda58xx_at_status ret;

    ret = _rda58xx.leGetConfigInfo(configInfo);
    _rda58xx.atHandler(ret);
}

int BleConfig::leGetConfigState(void)
{
    rda58xx_at_status ret;
	int8_t value = 0;
    ret = _rda58xx.leGetConfigState(&value);
    _rda58xx.atHandler(ret);
	return value;
}

int BleConfig::leGetIndState(void)
{
    rda58xx_at_status ret;
	int8_t value = 0;
    ret = _rda58xx.leGetIndState(&value);
    _rda58xx.atHandler(ret);
	return value;
}

int BleConfig::leClearConfigInfo(void)
{
    rda58xx_at_status ret;
	int8_t value = 0;
    ret = _rda58xx.leClearConfigInfo();
    _rda58xx.atHandler(ret);
	return value;
}


void BleConfig::leSetFeature(const char* state)
{
    rda58xx_at_status ret;
    ret = _rda58xx.leSetFeature(state);
    _rda58xx.atHandler(ret);
}


void BleConfig::leSetIndication(const char* state)
{
    rda58xx_at_status ret;
    ret = _rda58xx.leSetIndication(state);
    _rda58xx.atHandler(ret);
}


