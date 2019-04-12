// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Su Hao (suhao@baidu.com)
//
// Description: MediaAdapter for Rda58xx

#ifndef BAIDU_EXTERNAL_TARGETS_EXT_TARGET_UNO_91H_VS1053_BAIDU_MEDIA_ADAPTER_H
#define BAIDU_EXTERNAL_TARGETS_EXT_TARGET_UNO_91H_VS1053_BAIDU_MEDIA_ADAPTER_H

#include "baidu_codec_base.h"

namespace duer {

class MediaAdapter : public BaiduCodecBase {
public:
    MediaAdapter() :
        BaiduCodecBase() {
    }
};

} // namespace duer

#endif // BAIDU_EXTERNAL_TARGETS_EXT_TARGET_UNO_91H_VS1053_BAIDU_MEDIA_ADAPTER_H
