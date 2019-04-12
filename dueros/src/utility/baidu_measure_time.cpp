// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Zhang Guohua   (zhangguohua02@baidu.com)
//
// Description: Measure time utility

#include "baidu_measure_time.h"
#include <stdio.h>
#include <string>
#include "baidu_time_calculate.h"

namespace duer {

MeasureTime::MeasureTime(TimeFuncPtr time_func) {
    _s_time_func = time_func;
    std::memset(_MeasureArray, 0, sizeof(_MeasureArray));

    for (int i = 0; i < ArraySize; i++) {
        _MeasureArray[i].time_min = 0xffffffff;
    }
}

MeasureTime* MeasureTime::_s_instance = NULL;
TimeFuncPtr MeasureTime::_s_time_func = NULL;
MeasureTime* MeasureTime::getInstMeasureTime(TimeFuncPtr time_func) {
    if (NULL == _s_instance) {
        if (NULL == (_s_instance = new (std::nothrow) MeasureTime(time_func))) {
            printf("new fail.\r\n");
            return 0;
        }
    }

    return _s_instance;
}

void MeasureTime::addMeasureValue(int key, int count) {
    if (key < 0 || key >= ArraySize) {
        printf("key outside of the ArraySize.\r\n");
        return;
    }

    _MeasureArray[key].time_begin = _s_time_func();

    if (0 == _MeasureArray[key].total_cnt) {
        _MeasureArray[key].total_cnt = count;
    }
}

void MeasureTime::updateMeasureValue(int key, int arg_value,
                                     int continuous_measure_flag) {
    if (key < 0 || key >= ArraySize) {
        printf("key outside of the ArraySize.\r\n");
        return;
    }

    if ((continuous_measure_flag == UNCONTINUOUS_TIME_MEASURE)
            && (_MeasureArray[key].time_begin == 0)) {
        return;
    }

    _MeasureArray[key].arg_value += arg_value;

    if (_MeasureArray[key].cur_cnt >= _MeasureArray[key].total_cnt) {
        return;
    }

    _MeasureArray[key].cur_cnt++;
    unsigned int timeDelta = _s_time_func() - _MeasureArray[key].time_begin;
    _MeasureArray[key].time_sum += timeDelta;

    if (timeDelta < _MeasureArray[key].time_min) {
        _MeasureArray[key].time_min = timeDelta;
    }

    if (timeDelta > _MeasureArray[key].time_max) {
        _MeasureArray[key].time_max = timeDelta;
    }

    if (continuous_measure_flag == UNCONTINUOUS_TIME_MEASURE) {
        _MeasureArray[key].time_begin = 0;
    }
}

void MeasureTime::outputResult() {
    bool success = false;

    for (int i = 0; i < ArraySize; i++) {
        if (_MeasureArray[i].cur_cnt > 0) {
            printf("calculate time (key:%d, counter:%d), time( min:%d, max:%d, average:%f),"
					"(speed:%f)\r\n",
                   i, _MeasureArray[i].cur_cnt,
                   _MeasureArray[i].time_min, _MeasureArray[i].time_max,
                   (double)_MeasureArray[i].time_sum / _MeasureArray[i].cur_cnt,
                   (double)_MeasureArray[i].arg_value / _MeasureArray[i].time_sum);
            success = true;
        }
    }

    if (!success) {
        printf("please check TIME_BEGIN, TIME_END, with the arguement KEY, thanks.\r\n");
    }
}

extern "C" {

    void add_measure_value(int key, int count) {
        MeasureTime::getInstMeasureTime(us_ticker_read)->addMeasureValue(key, count);//TODO
    }

    void update_measure_value(int key, int arg_value, int continuous_measure_flag) {
        MeasureTime::getInstMeasureTime(us_ticker_read)->updateMeasureValue(key,
                arg_value,
                continuous_measure_flag);
    }

    void output_measure_result() {
        MeasureTime::getInstMeasureTime(us_ticker_read)->outputResult();
    }

} //extern "C"

} //namespace duer 

