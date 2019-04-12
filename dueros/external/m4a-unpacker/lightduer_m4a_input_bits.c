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
// Description: input bits from buffer

#include "lightduer_m4a_input_bits.h"
#include "lightduer_m4a_log.h"
#include "lightduer_m4a_unpacker.h"

static uint32_t duer_m4a_get_int32(const uint8_t *buff)
{
    return (buff[0] << 24) | (buff[1] << 16) | (buff[2] << 8) | buff[3];
}

/* reads only n bytes from the stream instead of the standard 4 */
static uint32_t duer_m4a_get_int32_n(const uint8_t *buff, int n)
{
    uint32_t ret = 0;
    if (n < 4) {
        for (int i = 0; i < n; i++) {
            ret |= buff[i] << ((3 - i) * 8);
        }
    }

    return ret;
}

static uint32_t duer_m4a_show_bits(duer_m4a_bit_context_t *bc, uint32_t bits)
{
    uint32_t ret = 0;
    if (bits <= bc->bits_left) {
        ret = (bc->first_buf << (32 - bc->bits_left)) >> (32 - bits);
    } else {
        bits -= bc->bits_left;

        ret = ((bc->first_buf & ((1 << bc->bits_left) - 1)) << bits)
                | (bc->second_buf >> (32 - bits));
    }

    return ret;
}

static void duer_m4a_flush_bits_ex(duer_m4a_bit_context_t *bc, uint32_t bits)
{
    bc->first_buf = bc->second_buf;
    if (bc->bytes_left >= 4) {
        bc->second_buf = duer_m4a_get_int32((uint8_t*)bc->tail);
        bc->bytes_left -= 4;
    } else {
        bc->second_buf = duer_m4a_get_int32_n((uint8_t*)bc->tail, bc->bytes_left);
        bc->bytes_left = 0;
    }
    bc->tail++;
    bc->bits_left += (32 - bits);
}

static void duer_m4a_flush_bits(duer_m4a_bit_context_t *bc, uint32_t bits)
{
    if (bits < bc->bits_left) {
        bc->bits_left -= bits;
    } else {
        duer_m4a_flush_bits_ex(bc, bits);
    }
}

/* return next n bits (right adjusted) */
uint32_t duer_m4a_get_bits(duer_m4a_bit_context_t *bc, uint32_t n)
{
    uint32_t ret = 0;

    if (n > 0 && n <= 32) {
        ret = duer_m4a_show_bits(bc, n);
        duer_m4a_flush_bits(bc, n);
    }

    return ret;
}

uint8_t duer_m4a_get_one_bit(duer_m4a_bit_context_t *bc)
{
    uint8_t ret = 0;

    if (bc->bits_left > 0) {
        bc->bits_left--;
        ret = (uint8_t)((bc->first_buf >> bc->bits_left) & 1);
    } else {
        /* bits_left == 0 */
        ret = (uint8_t)duer_m4a_get_bits(bc, 1);
    }

    return ret;
}

/* initialize buffer, call once before first getbits or showbits */
int32_t duer_m4a_init_input_bits(duer_m4a_bit_context_t *bc,
        const uint8_t *buffer, uint32_t buffer_size)
{
    if (bc == NULL || buffer_size == 0 || buffer == NULL) {
        LOGE("invalid arguments");
        return M4A_ERROR_UNSPECIFIELD;
    }

    bc->buffer = buffer;

    bc->buffer_size = buffer_size;
    bc->bytes_left  = buffer_size;

    if (bc->bytes_left >= 4) {
        bc->first_buf = duer_m4a_get_int32(bc->buffer);
        bc->bytes_left -= 4;
    } else {
        bc->first_buf = duer_m4a_get_int32_n(bc->buffer, bc->bytes_left);
        bc->bytes_left = 0;
    }

    if (bc->bytes_left >= 4) {
        bc->second_buf = duer_m4a_get_int32(bc->buffer + sizeof(uint32_t));
        bc->bytes_left -= 4;
    } else {
        bc->second_buf = duer_m4a_get_int32_n(bc->buffer + sizeof(uint32_t), bc->bytes_left);
        bc->bytes_left = 0;
    }

    bc->tail = (uint32_t*)bc->buffer + 2;

    bc->bits_left = 32;

    return M4A_ERROR_OK;
}

void duer_m4a_end_input_bits(duer_m4a_bit_context_t *bc)
{
    // do nothing
}

uint32_t duer_m4a_get_processed_bits(duer_m4a_bit_context_t *bc)
{
    // 32 bits per uint32_t
    return (uint32_t)(32 * (bc->tail - (uint32_t*)bc->buffer - 1) - bc->bits_left);
}
