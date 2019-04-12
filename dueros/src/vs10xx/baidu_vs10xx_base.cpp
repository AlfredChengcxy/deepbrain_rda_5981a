// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Su Hao (suhao@baidu.com)
//
// Description: Implement MediaBase for Vs10xx

#include "baidu_vs10xx_base.h"
#include "lightduer_log.h"

namespace duer
{

Vs10xxBase::Vs10xxBase(
    PinName mosi,
    PinName miso,
    PinName sclk,
    PinName xcs,
    PinName xdcs,
    PinName dreq,
    PinName xreset) :
    _vs10xx(mosi, miso, sclk, xcs, xdcs, dreq, xreset),
    _play_stopped(true)
{
}

int Vs10xxBase::on_start_play(MediaType type, int bitrate)
{
    DUER_LOGV("[%s] begin", __func__);
    _vs10xx.softReset();
    _play_stopped = false;
    _vs10xx.setFreq(10000000);
    DUER_LOGV("[%s] end", __func__);
    return 0;
}

int Vs10xxBase::on_write(const void* data, size_t size)
{
    DUER_LOGV("[%s] begin", __func__);

    static const int MP3_FRAME_SIZE = 32;

    if (data == NULL || size == 0) {
        return -1;
    }
    unsigned char *ptr_data = (unsigned char *)data;
    int data_size = size;
    int bytes = 0;
    while (data_size > 0) {
        bytes = (data_size > MP3_FRAME_SIZE) ? MP3_FRAME_SIZE : data_size;
        _vs10xx.writeData((unsigned char*)ptr_data, bytes);
        data_size -= bytes;
        ptr_data += bytes;
    }

    DUER_LOGV("[%s] end", __func__);
    return 0;
}

int Vs10xxBase::on_voice(unsigned char vol)
{
    return 0;
}

int Vs10xxBase::on_pause_play()
{
    return 0;
}

int Vs10xxBase::on_stop_play()
{
    DUER_LOGV("[%s] begin", __func__);
    _vs10xx.softReset();
    _play_stopped = true;
    DUER_LOGV("[%s] end", __func__);
    return 0;
}

int Vs10xxBase::on_start_record()
{
    DUER_LOGV("[%s] begin", __func__);
    _vs10xx.softReset();
    _play_stopped = true;

    _vs10xx.writeRegister(SPI_BASS, 0x0055);        //set accent
    _vs10xx.writeRegister(SPI_AICTRL0, 8000);       //samplerate 8k
    _vs10xx.writeRegister(SPI_AICTRL1, 64 * 1024);  //record gain
    _vs10xx.writeRegister(SPI_AICTRL2, 64 * 1024);  //Set the gain maximum,65536=64X
    _vs10xx.writeRegister(SPI_AICTRL3, 6);          //PCM Mode ,right channel,board mic
    _vs10xx.writeRegister(SPI_CLOCKF, 0x8800);      //set clock
    _vs10xx.writeRegister(SPI_MODE, 0x1804);        //Initialize recording
    _vs10xx.loadPlugin(recPlugin, sizeof(recPlugin) / sizeof(recPlugin[0]));
    _vs10xx.setVolume(0xFE);
    _vs10xx.setFreq(5000000);

#if defined(TASK_SLEEP_DELAY) && (TASK_SLEEP_DELAY > 0)
    _begin = _timer.read_ms();
#endif
    DUER_LOGV("[%s] end", __func__);
    return 0;
}

size_t Vs10xxBase::on_read(void* data, size_t size)
{
    DUER_LOGV("[%s] begin", __func__);
    char* p_cur = (char*)data;
    unsigned int reg = 0;
    unsigned int len = _vs10xx.readRegister(SPI_HDAT1);

#if defined(TASK_SLEEP_DELAY) && (TASK_SLEEP_DELAY > 0)
    int delta = _timer.read_ms() - _begin;
    if (delta < TASK_SLEEP_DELAY && delta >= 0) {
        Thread::wait(TASK_SLEEP_DELAY - delta);
    }
#endif

    if (len * 2 > size) {
        len = size / 2;
    }

    while (len != 0) {
        reg = _vs10xx.readRegister(SPI_HDAT0);
        *p_cur++ = (unsigned char)(reg & 0xFF);
        *p_cur++ = (unsigned char)(reg >> 8);
        len--;
    }

#if defined(TASK_SLEEP_DELAY) && (TASK_SLEEP_DELAY > 0)
    _begin = _timer.read_ms();
#endif

    DUER_LOGV("[%s] end", __func__);
    return p_cur - (char*)data;
}

int Vs10xxBase::on_stop_record()
{
    DUER_LOGV("[%s] begin", __func__);
    _vs10xx.setFreq(1000000);     //low speed
    _vs10xx.softReset();
    DUER_LOGV("[%s] end", __func__);
    return 0;
}

bool Vs10xxBase::is_play_stopped_internal()
{
    return _play_stopped;
}

} // namespace duer
