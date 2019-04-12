/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: parse common m4a(mp4) boxes, internal API

#ifndef BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_BOX_PARSER_H
#define BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_BOX_PARSER_H

#include "lightduer_m4a_unpacker.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parse m4a box, only handle some common box now
 *
 * @Param unpacker, m4a unpacker context
 * @Return success: 0, fail: < 0
 */
int32_t duer_m4a_parse_boxes(duer_m4a_unpacker_t *unpacker);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_BOX_PARSER_H
