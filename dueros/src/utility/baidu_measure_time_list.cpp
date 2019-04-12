// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Zhang Guohua(zhangguohua02@baidu.com)
//
// Description: Measure time list

#include "baidu_measure_time_list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace duer {

MeasureTimeList::MeasureTimeList(TimeFuncPtr time_func) : _pListHead(NULL),
        _pListTail(NULL) {
    _s_time_func = time_func;
}
//MeasureTimeList::MeasureTimeList(const MeasureTime&){}
//MeasureTimeList& MeasureTimeList::operator=(const MeasureTimeList&){ return *this;}
MeasureTimeList* MeasureTimeList::_s_instance = NULL;
TimeFuncPtr MeasureTimeList::_s_time_func = NULL;
MeasureTimeList* MeasureTimeList::getInstMeasureTime(TimeFuncPtr time_func) {
    if (NULL == _s_instance) {
        if (NULL == (_s_instance = new (std::nothrow) MeasureTimeList(time_func))) {
            printf("new fail.");
            return NULL;
        }
    }

    return _s_instance;
}

void MeasureTimeList::addMeasureValue(const char* key, unsigned int count) {
    if (NULL == key) {
        printf("key NULL.");
        return;
    }

    ListNode* temp = NULL;

    // not find
    if (false == searchMeasureValueByKey(key, temp)) {
        ListNode* pListNode = (ListNode*)malloc(sizeof(ListNode));

        if (NULL == pListNode) {
            printf("malloc fail.");
            return;
        }

        memset(pListNode, 0, sizeof(ListNode));
        pListNode->next = NULL;
        pListNode->Key = key;
        pListNode->Value.total_cnt = count;
        pListNode->Value.cur_cnt = 0;
        pListNode->Value.time_begin = _s_time_func();
        pListNode->Value.time_sum = 0;
        pListNode->Value.time_min = 0xffffffff;
        pListNode->Value.time_max = 0;

        if (NULL == _pListHead && NULL == _pListTail) {
            _pListHead = _pListTail = pListNode;
        } else {
            _pListTail->next = pListNode;
            _pListTail = pListNode;
        }

        return;
    }

    // find ok
    if (NULL != temp) {
        temp->Value.time_begin = _s_time_func();
    } else {
        printf("temp NULL.");
    }

    return;
}

void MeasureTimeList::updateMeasureValue(const char* key, unsigned arg_value) {
    if (NULL == key) {
        printf("key NULL.");
        return;
    }

    ListNode* temp = NULL;

    // not find
    if (false == searchMeasureValueByKey(key, temp)) {
        printf("key error.");
        return;
    }

    // find ok
    if (NULL == temp) {
        printf("temp NULL.");
        return;
    }

    if (temp->Value.cur_cnt >= temp->Value.total_cnt) {
        return;
    }

    temp->Value.cur_cnt++;
    temp->Value.arg_value += arg_value;
    unsigned int timeDelta = _s_time_func() - temp->Value.time_begin;
    temp->Value.time_sum += timeDelta;

    if (timeDelta < temp->Value.time_min) {
        temp->Value.time_min = timeDelta;
    }

    if (timeDelta > temp->Value.time_max) {
        temp->Value.time_max = timeDelta;
    }
}

bool MeasureTimeList::searchMeasureValueByKey(const char* key,
        ListNode* &temp) {
    if (NULL == key || NULL == _pListHead) {
        return false;
    }

    ListNode* pListNodeTemp = _pListHead;

    while (NULL != pListNodeTemp && (0 != pListNodeTemp->Key.compare(key))) {
        pListNodeTemp = pListNodeTemp->next;
    }

    if (NULL == pListNodeTemp) {
        temp = NULL;
        return false;
    } else {
        temp = pListNodeTemp;
        return true;
    }
}

void MeasureTimeList::outputResult() {
    bool success = false;
    ListNode* temp = _pListHead;

    while (NULL != temp) {
        if (temp->Value.cur_cnt > 0) {
            printf("measure time (key:%s, counter:%d), time( min:%d, max:%d, average:%f),"
                   "(speed:%f)\r\n",
                   temp->Key.c_str(), temp->Value.cur_cnt,
                   temp->Value.time_min, temp->Value.time_max,
                   (double)temp->Value.time_sum / temp->Value.cur_cnt,
                   (double)temp->Value.arg_value / temp->Value.time_sum);
            success = true;
        }

        temp = temp->next;
    }

    if (!success) {
        printf("please check TIME_BEGIN, TIME_END, with the arguement KEY, thanks.\r\n");
    }
}

void MeasureTimeList::destoryMeasureValueList() {
    if (NULL == _pListHead) {
        return;
    }

    ListNode* pListNodeCurrent = _pListHead; 
    ListNode* pListNodeTemp = NULL;
    _pListHead = NULL;

    while (NULL != pListNodeCurrent) {
        pListNodeTemp = pListNodeCurrent->next;
        free(pListNodeCurrent);
        pListNodeCurrent = pListNodeTemp;
    }
}

} //namespace duer 

