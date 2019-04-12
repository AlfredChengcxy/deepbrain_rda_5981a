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
// Description: parse audio specific configuration, external API

#ifndef BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_AUDIO_SPECIFIC_CONFIG_H
#define BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_AUDIO_SPECIFIC_CONFIG_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* lc-aac decoder can decode he-aac(ignore SBR), so open this macro */
#define SBR_DEC
/* if aac decoder supports PS, open this macro */
// #define PS_DEC

#define ADTS_HEAD_SIZE 7

/*
 * audio specific configuration structure
 */
typedef struct duer_m4a_audio_specific_config_s {
    uint8_t  object_type_index;
    uint8_t  channels_configuration;
    uint8_t  sampling_frequency_index;
    uint32_t sampling_frequency;

    int8_t   sbr_present_flag;
    int8_t   ps_present_flag;
    int8_t   force_up_sampling;
    int8_t   down_sampled_sbr;
} duer_m4a_audio_specific_config_t;

/*
 * Parse aac audio specific config
 *
 * @Param buffer, source data
 * @Param buffer_size, the size of source data
 * @Param m4a_asc, save result to this object
 * @Param short_form, indicate whether parse extension info
 * @Return success: 0, fail: < 0
 */
int8_t duer_m4a_get_audio_specific_config(uint8_t *buffer, uint32_t buffer_size,
        duer_m4a_audio_specific_config_t *m4a_asc, uint8_t short_form);

/*
 * make adts header(7 bytes)
 *
 * @Param data, write adts header to data, size of it must bigger than 7 bytes
 * @Param frame_size, the size of aac frame
 * @Param m4a_asc, audio specific configuration
 * @Return success: 0, fail: < 0
 */
int32_t duer_m4a_make_adts_header(uint8_t *data, int frame_size,
        duer_m4a_audio_specific_config_t *m4a_asc);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_AUDIO_SPECIFIC_CONFIG_H

