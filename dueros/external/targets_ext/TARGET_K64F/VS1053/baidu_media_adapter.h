// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Su Hao (suhao@baidu.com)
//
// Description: MediaAdapter for Vs10xx

#ifndef BAIDU_EXTERNAL_TARGETS_EXT_TARGET_K64F_VS1053_BAIDU_MEDIA_ADAPTER_H
#define BAIDU_EXTERNAL_TARGETS_EXT_TARGET_K64F_VS1053_BAIDU_MEDIA_ADAPTER_H

#include "baidu_vs10xx_base.h"

namespace duer {

class MediaAdapter : public Vs10xxBase {
public:
    MediaAdapter() :
        Vs10xxBase(D11, D12, D13, A3, A2, A1, A0) {
    }

};

} // namespace duer

#endif // BAIDU_EXTERNAL_TARGETS_EXT_TARGET_K64F_VS1053_BAIDU_MEDIA_ADAPTER_H
