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
// Description: read data from m4a file, internal API

#ifndef BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_UTIL_H
#define BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_UTIL_H

#include "lightduer_m4a_unpacker.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Read specific size data from m4a file
 *
 * @Param unpacker, m4a unpacker context
 * @Param data, destination buffer
 * @Param size, bytes to read
 * @Return actual read size
 */
uint32_t duer_m4a_read_data(duer_m4a_unpacker_t *unpacker, uint8_t *data, uint32_t size);

/*
 * Set the position of m4a file
 *
 * @Param unpacker, m4a unpacker context
 * @Param position, the position to seek
 * @Return success: 0, fail: < 0
 */
int32_t duer_m4a_set_position(duer_m4a_unpacker_t *unpacker, const int64_t position);

/*
 * Get the position of m4a file
 *
 * @Param unpacker, m4a unpacker context
 * @Return current position of m4a file
 */
int64_t duer_m4a_position(const duer_m4a_unpacker_t *unpacker);

/*
 * Read one int64
 *
 * @Param unpacker, m4a unpacker context
 * @Return int64 value
 */
uint64_t duer_m4a_read_int64(duer_m4a_unpacker_t *unpacker);

/*
 * Read one int32
 *
 * @Param unpacker, m4a unpacker context
 * @Return int32 value
 */
uint32_t duer_m4a_read_int32(duer_m4a_unpacker_t *unpacker);

/*
 * Read one int24
 *
 * @Param unpacker, m4a unpacker context
 * @Return int24 value
 */
uint32_t duer_m4a_read_int24(duer_m4a_unpacker_t *unpacker);

/*
 * Read one int16
 *
 * @Param unpacker, m4a unpacker context
 * @Return int16 value
 */
uint16_t duer_m4a_read_int16(duer_m4a_unpacker_t *unpacker);

/*
 * Read specific length string
 *
 * @Param unpacker, m4a unpacker context
 * @Param length, the length to read
 * @Return string point
 */
char * duer_m4a_read_string(duer_m4a_unpacker_t *unpacker, uint32_t length);

/*
 * Read one byte
 *
 * @Param unpacker, m4a unpacker context
 * @Return byte value
 */
uint8_t duer_m4a_read_byte(duer_m4a_unpacker_t *unpacker);

/*
 * Read the length of description in m4a box
 *
 * @Param unpacker, m4a unpacker context
 * @Return the length of description
 */
uint32_t duer_m4a_read_m4a_descr_length(duer_m4a_unpacker_t *unpacker);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_UTIL_H
