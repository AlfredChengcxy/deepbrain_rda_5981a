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
// Description: read data from m4a file

#include "lightduer_m4a_util.h"
#include "lightduer_m4a_log.h"
#include "heap_monitor.h"

uint32_t duer_m4a_read_data(duer_m4a_unpacker_t *unpacker, uint8_t *data, uint32_t size)
{
    uint32_t result = unpacker->stream->read(unpacker->stream->user_data, data, size);
    if (result != size) {
        LOGW("duer_m4a_read_data read %d bytes, expect %d bytes", result, size);
    }
    unpacker->current_position += size;

    return result;
}

int32_t duer_m4a_set_position(duer_m4a_unpacker_t *unpacker, const int64_t position)
{
    unpacker->stream->seek(unpacker->stream->user_data, position);
    unpacker->current_position = position;

    return M4A_ERROR_OK;
}

int64_t duer_m4a_position(const duer_m4a_unpacker_t *unpacker)
{
    return unpacker->current_position;
}

uint64_t duer_m4a_read_int64(duer_m4a_unpacker_t *unpacker)
{
    uint8_t data[8];
    uint64_t result = 0;

    duer_m4a_read_data(unpacker, data, 8);

    for (int8_t i = 0; i < 8; i++) {
        result |= ((uint64_t)data[i]) << ((7 - i) * 8);
    }

    return result;
}

uint32_t duer_m4a_read_int32(duer_m4a_unpacker_t *unpacker)
{
    uint8_t data[4];
    uint32_t result = 0;

    duer_m4a_read_data(unpacker, data, 4);

    result = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

    return result;
}

uint32_t duer_m4a_read_int24(duer_m4a_unpacker_t *unpacker)
{
    uint8_t data[3];
    uint32_t result = 0;

    duer_m4a_read_data(unpacker, data, 3);

    result = (data[0] << 16) | (data[1] << 8) | data[2];

    return result;
}

uint16_t duer_m4a_read_int16(duer_m4a_unpacker_t *unpacker)
{
    uint8_t data[2];
    uint16_t result = 0;

    duer_m4a_read_data(unpacker, data, 2);

    result = (data[0] << 8) | data[1];

    return result;
}

char * duer_m4a_read_string(duer_m4a_unpacker_t *unpacker, uint32_t length)
{
    char *str = (char* )MALLOC(length + 1, MEDIA);
    if (str != NULL) {
        if (duer_m4a_read_data(unpacker, (uint8_t*)str, length) != length) {
            FREE(str);
            str = NULL;
        } else {
            str[length] = 0;
        }
    }

    return str;
}

uint8_t duer_m4a_read_byte(duer_m4a_unpacker_t *unpacker)
{
    uint8_t output;
    duer_m4a_read_data(unpacker, &output, 1);
    return output;
}

uint32_t duer_m4a_read_m4a_descr_length(duer_m4a_unpacker_t *unpacker)
{
    uint8_t b;
    uint8_t numBytes = 0;
    uint32_t length = 0;

    do {
        b = duer_m4a_read_byte(unpacker);
        numBytes++;
        length = (length << 7) | (b & 0x7F);
    } while ((b & 0x80) && numBytes < 4);

    return length;
}
