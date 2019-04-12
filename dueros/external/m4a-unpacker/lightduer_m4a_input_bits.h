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
// Description: input bits from buffer, internal API

#ifndef BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_UTIL_H
#define BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_UTIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * context for bit input
 */
typedef struct duer_m4a_bit_context_s {
    uint32_t        first_buf;   /* first 4 bytes buffer */
    uint32_t        second_buf;  /* second 4 bytes buffer */
    uint32_t        bits_left;   /* left bits of first_buf */
    uint32_t        bytes_left;
    uint32_t        *tail;       /* tail of the buffer that has been readed to buf*/
    uint32_t        buffer_size; /* size of the buffer in bytes */
    const uint8_t   *buffer;     /* data buffer */
} duer_m4a_bit_context_t;

/*
 * Initialize bits input
 *
 * @Param bc, bits input context
 * @Param buffer, source data
 * @Param buffer_size, size of buffer
 * @Return success: 0, fail: < 0
 */
int32_t duer_m4a_init_input_bits(duer_m4a_bit_context_t *bc,
        const uint8_t *buffer, uint32_t buffer_size);

/*
 * End bits input
 */
void duer_m4a_end_input_bits(duer_m4a_bit_context_t *bc);

/*
 * Get next n bits (right adjusted)
 *
 * @Param bc, bits input context
 * @Param n, bits to get, 32 at most
 * @Return uint32_t value include bits
 */
uint32_t duer_m4a_get_bits(duer_m4a_bit_context_t *bc, uint32_t n);

/*
 * Get next one bit
 *
 * @Param bc, bits input context
 * @Return uint8_t value include one bit
 */
uint8_t duer_m4a_get_one_bit(duer_m4a_bit_context_t *bc);

/*
 * Get the number of bits have been processed
 *
 * @Param bc, bits input context
 * @Return the number of bits
 */
uint32_t duer_m4a_get_processed_bits(duer_m4a_bit_context_t *bc);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_UTIL_H
