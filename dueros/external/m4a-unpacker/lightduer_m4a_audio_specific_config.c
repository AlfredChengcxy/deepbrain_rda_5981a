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
// Description: parse audio specific configuration

#include "lightduer_m4a_audio_specific_config.h"
#include <stdio.h>
#include <string.h>
#include "lightduer_m4a_unpacker.h"
#include "lightduer_m4a_input_bits.h"
#include "lightduer_m4a_log.h"

#define MAX_CHANNELS_SUPPORTS 2

/* index of OBJECT_TYPES_TABLE */
#define OBJECT_TYPE_AAC_MAIN        1
#define OBJECT_TYPE_AAC_LC          2
#define OBJECT_TYPE_AAC_SSR         3
#define OBJECT_TYPE_AAC_LTP         4
#define OBJECT_TYPE_SBR             5
#define OBJECT_TYPE_AAC_SCALABLE    6
#define OBJECT_TYPE_TWIN_VQ         7
#define OBJECT_TYPE_PS              29

/* defines if an object type can be decoded by this library or not */
static const uint8_t OBJECT_TYPES_TABLE[32] = {
    0, /*  0 NULL */
    0, /*  1 AAC Main */
    1, /*  2 AAC LC */
    0, /*  3 AAC SSR */
    0, /*  4 AAC LTP */
#ifdef SBR_DEC
    1, /*  5 SBR */
#else
    0, /*  5 SBR */
#endif
    0, /*  6 AAC Scalable */
    0, /*  7 TwinVQ */
    0, /*  8 CELP */
    0, /*  9 HVXC */
    0, /* 10 Reserved */
    0, /* 11 Reserved */
    0, /* 12 TTSI */
    0, /* 13 Main synthetic */
    0, /* 14 Wavetable synthesis */
    0, /* 15 General MIDI */
    0, /* 16 Algorithmic Synthesis and Audio FX */

    /* MPEG-4 Version 2 */
    0, /* 17 ER AAC LC */
    0, /* 18 (Reserved) */
    0, /* 19 ER AAC LTP */
    0, /* 20 ER AAC scalable */
    0, /* 21 ER TwinVQ */
    0, /* 22 ER BSAC */
    0, /* 23 ER AAC LD */
    0, /* 24 ER CELP */
    0, /* 25 ER HVXC */
    0, /* 26 ER HILN */
    0, /* 27 ER Parametric */
    0, /* 28 (Reserved) */
    1, /* 29 PS */
    0, /* 30 (Reserved) */
    0  /* 31 (Reserved) */
};

static const uint32_t SAMPLE_RATES_COUNT = 12;
static const uint32_t SAMPLE_RATES[] = {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025, 8000
    };

static const uint32_t ADTS_SAMPLE_RATES_COUNT = 16;
static const uint32_t ADTS_SAMPLE_RATES[] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000,
        22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0};

/* Returns the sample rate based on the sample rate index */
static uint32_t duer_m4a_get_sample_rate(const uint8_t sr_index)
{
    if (sr_index < SAMPLE_RATES_COUNT) {
        return SAMPLE_RATES[sr_index];
    }

    return 0;
}

static int8_t duer_m4a_parse_ga_specific_config(duer_m4a_bit_context_t *bc,
        duer_m4a_audio_specific_config_t *m4a_asc)
{
    /* GA Specific Info */
    uint8_t frame_length_flag = 0;
    uint8_t depends_on_core_coder = 0;
    uint16_t core_coder_delay = 0;
    uint8_t extension_flag = 0;

    /* 1024 or 960 */
    frame_length_flag = duer_m4a_get_one_bit(bc);
    if (frame_length_flag == 1) {
        return M4A_ERROR_UNSPECIFIELD;
    }

    depends_on_core_coder = duer_m4a_get_one_bit(bc);
    if (depends_on_core_coder == 1) {
        core_coder_delay = (uint16_t)duer_m4a_get_bits(bc, 14);
    }

    extension_flag = duer_m4a_get_one_bit(bc);

    LOGD("GA specific info: frame_length_flag = %d, depends_on_core_coder = %d,"
            "core_coder_delay = %d, extension_flag = %d", frame_length_flag,
            depends_on_core_coder, core_coder_delay, extension_flag);

    return M4A_ERROR_OK;
}

static uint8_t duer_m4a_find_adts_sample_rate_index(uint32_t sr)
{
    for (uint8_t i = 0; i < ADTS_SAMPLE_RATES_COUNT; i++) {
        if (sr == ADTS_SAMPLE_RATES[i]) {
            return i;
        }
    }

    return ADTS_SAMPLE_RATES_COUNT - 1;
}

static int8_t duer_m4a_parse_audio_specific_config(duer_m4a_bit_context_t *bc,
        duer_m4a_audio_specific_config_t *m4a_asc, uint32_t buffer_size, uint8_t short_form)
{
    int8_t ret = 0;
    uint32_t start_pos = duer_m4a_get_processed_bits(bc);
#ifdef SBR_DEC
    int8_t bits_to_decode = 0;
    int8_t orig_object_type_index = 0;
#endif

    memset(m4a_asc, 0, sizeof(*m4a_asc));

    m4a_asc->object_type_index = (uint8_t)duer_m4a_get_bits(bc, 5);

    m4a_asc->sampling_frequency_index = (uint8_t)duer_m4a_get_bits(bc, 4);
    if (m4a_asc->sampling_frequency_index == 0x0f) {
        duer_m4a_get_bits(bc, 24); /* skip 24 bits */
    }

    m4a_asc->sampling_frequency = duer_m4a_get_sample_rate(m4a_asc->sampling_frequency_index);

    m4a_asc->channels_configuration = (uint8_t)duer_m4a_get_bits(bc, 4);

    LOGD("object_type_index=%d, sampling_frequency_index=%d, channels_configuration=%d,"
            "sbr_present_flag=%d", m4a_asc->object_type_index, m4a_asc->sampling_frequency_index,
            m4a_asc->channels_configuration, m4a_asc->sbr_present_flag);

    do {
        if (OBJECT_TYPES_TABLE[m4a_asc->object_type_index] != 1) {
            LOGE("unsurpport object type %d", m4a_asc->object_type_index);
            ret = M4A_ERROR_UNSPECIFIELD;
            break;
        }

        if (m4a_asc->sampling_frequency == 0) {
            LOGE("invalid sampling frequency");
            ret = M4A_ERROR_UNSPECIFIELD;
            break;
        }

        if (m4a_asc->channels_configuration == 0) {
            LOGE("invalid channels configuration");
            ret = M4A_ERROR_UNSPECIFIELD;
            break;
        }

        if (m4a_asc->channels_configuration > MAX_CHANNELS_SUPPORTS) {
            LOGE("unsurpport channels %d", m4a_asc->channels_configuration);
            ret = M4A_ERROR_UNSPECIFIELD;
            break;
        }

#if (defined(PS_DEC) || defined(DRM_PS))
        /* check if we have a mono file */
        if (m4a_asc->channels_configuration == 1) {
            /* upMatrix to 2 channels for implicit signalling of PS */
            m4a_asc->channels_configuration = 2;
        }
#endif

#ifdef SBR_DEC
        m4a_asc->sbr_present_flag = -1;
        m4a_asc->ps_present_flag = -1;
        orig_object_type_index = m4a_asc->object_type_index;

        if (m4a_asc->object_type_index == OBJECT_TYPE_SBR
                || m4a_asc->object_type_index == OBJECT_TYPE_PS) {
            uint8_t tmp = 0;

            m4a_asc->sbr_present_flag = 1;
            if (m4a_asc->object_type_index == OBJECT_TYPE_PS) {
                m4a_asc->ps_present_flag = 1;
            }

            tmp = (uint8_t)duer_m4a_get_bits(bc, 4);
            /* check for downsampled SBR */
            if (tmp == m4a_asc->sampling_frequency_index) {
                m4a_asc->down_sampled_sbr = 1;
            }

            m4a_asc->sampling_frequency_index = tmp;
            if (m4a_asc->sampling_frequency_index == 0x0f) {
                m4a_asc->sampling_frequency = (uint32_t)duer_m4a_get_bits(bc, 24);
            } else {
                m4a_asc->sampling_frequency = duer_m4a_get_sample_rate(
                        m4a_asc->sampling_frequency_index);
            }

            m4a_asc->object_type_index = (uint8_t)duer_m4a_get_bits(bc, 5);
        }

        LOGD("2 object_type_index=%d, sampling_frequency_index=%d, channels_configuration=%d,"
                "sbr_present_flag=%d", m4a_asc->object_type_index, m4a_asc->sampling_frequency_index,
                m4a_asc->channels_configuration, m4a_asc->sbr_present_flag);

        if (OBJECT_TYPES_TABLE[m4a_asc->object_type_index] != 1) {
            LOGE("unsurpport object type %d", m4a_asc->object_type_index);
            ret = M4A_ERROR_UNSPECIFIELD;
            break;
        }
#endif /* SBR_DEC */

        /* get GASpecificConfig */
        if (m4a_asc->object_type_index == OBJECT_TYPE_AAC_MAIN
                || m4a_asc->object_type_index == OBJECT_TYPE_AAC_LC
                || m4a_asc->object_type_index == OBJECT_TYPE_AAC_SSR
                || m4a_asc->object_type_index == OBJECT_TYPE_AAC_LTP
                || m4a_asc->object_type_index == OBJECT_TYPE_AAC_SCALABLE
                || m4a_asc->object_type_index == OBJECT_TYPE_TWIN_VQ) {
            ret = duer_m4a_parse_ga_specific_config(bc, m4a_asc);
            if (ret < 0) {
                LOGE("parse ga specific config failed");
                break;
            }
        } else {
            LOGE("unsurpport object type:%d", m4a_asc->object_type_index);
            ret = M4A_ERROR_UNSPECIFIELD;
            break;
        }


#ifdef SBR_DEC
        if (short_form) {
            bits_to_decode = 0;
        } else {
            bits_to_decode = (int8_t)(buffer_size * 8 - (start_pos - duer_m4a_get_processed_bits(bc)));
        }
        LOGD("bits_to_decode=%d\n", bits_to_decode);

        /* sync extension info needs 16 bits at least */
        if ((m4a_asc->object_type_index != OBJECT_TYPE_SBR) && (bits_to_decode >= 16)) {
            int16_t sync_extension_type = (int16_t)duer_m4a_get_bits(bc, 11);
            LOGD("sync_extension_type = 0x%04x\n", sync_extension_type);

            if (sync_extension_type == 0x2b7) {
                uint8_t tmp_oti = (uint8_t)duer_m4a_get_bits(bc, 5);

                if (tmp_oti == OBJECT_TYPE_SBR) {
                    m4a_asc->sbr_present_flag = (uint8_t)duer_m4a_get_one_bit(bc);

                    if (m4a_asc->sbr_present_flag) {
                        uint8_t tmp = 0;

                        /* Don't set OT to SBR until checked that it is actually there */
                        m4a_asc->object_type_index = tmp_oti;

                        tmp = (uint8_t)duer_m4a_get_bits(bc, 4);

                        /* check for downsampled SBR */
                        if (tmp == m4a_asc->sampling_frequency_index) {
                            m4a_asc->down_sampled_sbr = 1;
                        }
                        m4a_asc->sampling_frequency_index = tmp;

                        if (m4a_asc->sampling_frequency_index == 0x0f) {
                            m4a_asc->sampling_frequency = (uint32_t)duer_m4a_get_bits(bc, 24);
                        } else {
                            m4a_asc->sampling_frequency = duer_m4a_get_sample_rate(
                                    m4a_asc->sampling_frequency_index);
                        }
                        bits_to_decode = (int8_t)(buffer_size * 8 - (start_pos
                                            - duer_m4a_get_processed_bits(bc)));
                        LOGD("bits_to_decode=%d\n", bits_to_decode);
                        if (bits_to_decode >= 12) {
                            sync_extension_type = (int16_t)duer_m4a_get_bits(bc, 11);
                            LOGD("sync_extension_type = 0x%04x\n", sync_extension_type);
                            if (sync_extension_type == 0x548) {
                                m4a_asc->ps_present_flag = (uint8_t)duer_m4a_get_one_bit(bc);
                            }
                        }
                    }
                }
            }
        }

        /* no SBR signalled, this could mean either implicit signalling or no SBR in this file */
        /* MPEG specification states: assume SBR on files with samplerate <= 24000 Hz */
        if (m4a_asc->sbr_present_flag == -1) {
            if (m4a_asc->sampling_frequency <= 24000) {
                m4a_asc->sampling_frequency *= 2;
                m4a_asc->force_up_sampling = 1;
            } else { /* > 24000*/
                m4a_asc->down_sampled_sbr = 1;
            }
        }
        LOGD("3 object_type_index=%d, sampling_frequency_index=%d, channels_configuration=%d,"
                "sbr_present_flag=%d", m4a_asc->object_type_index, m4a_asc->sampling_frequency_index,
                m4a_asc->channels_configuration, m4a_asc->sbr_present_flag);

        if (OBJECT_TYPES_TABLE[m4a_asc->object_type_index] != 1) {
            LOGE("unsurpport object type %d", m4a_asc->object_type_index);
            ret = M4A_ERROR_UNSPECIFIELD;
            break;
        }

        if (m4a_asc->sbr_present_flag == 1) {
            m4a_asc->sampling_frequency = m4a_asc->sampling_frequency / 2;
            m4a_asc->sampling_frequency_index =
                    duer_m4a_find_adts_sample_rate_index(m4a_asc->sampling_frequency);

            if (m4a_asc->object_type_index == 5) {
                m4a_asc->object_type_index = orig_object_type_index;
            }
        }

        LOGD("4 object_type_index=%d, sampling_frequency_index=%d, channels_configuration=%d,"
                "sbr_present_flag=%d", m4a_asc->object_type_index, m4a_asc->sampling_frequency_index,
                m4a_asc->channels_configuration, m4a_asc->sbr_present_flag);

        if (OBJECT_TYPES_TABLE[m4a_asc->object_type_index] != 1) {
            LOGE("unsurpport object type %d", m4a_asc->object_type_index);
            ret = M4A_ERROR_UNSPECIFIELD;
            break;
        }
#endif /* SBR_DEC */
    } while (0);

    return ret;
}

int8_t duer_m4a_get_audio_specific_config(uint8_t *buffer, uint32_t buffer_size,
        duer_m4a_audio_specific_config_t *m4a_asc, uint8_t short_form)
{
    int8_t ret = 0;
    duer_m4a_bit_context_t bc;

    if (buffer == NULL || buffer_size <= 0 || m4a_asc == NULL) {
        LOGE("invalid arguments");
        return M4A_ERROR_UNSPECIFIELD;
    }

    ret = duer_m4a_init_input_bits(&bc, buffer, buffer_size);
    if (ret < 0) {
        LOGE("init input bits failed");
        return ret;
    }

    ret = duer_m4a_parse_audio_specific_config(&bc, m4a_asc, buffer_size, short_form);

    duer_m4a_end_input_bits(&bc);

    return ret;
}

int32_t duer_m4a_make_adts_header(uint8_t *data, int frame_size,
        duer_m4a_audio_specific_config_t *m4a_asc)
{
    if (data == NULL || m4a_asc == NULL) {
        LOGE("invalid arguments");
        return M4A_ERROR_UNSPECIFIELD;
    }

    int profile = (m4a_asc->object_type_index - 1) & 0x3;

    memset(data, 0, ADTS_HEAD_SIZE);

    /* 12bits: syncword */
    data[0] += 0xFF;
    data[1] += 0xF0;

    /* 1bits: mpeg ID = 0 */
    /* 2bits: layer = 0 */
    /* 1bits: protection absent */
    data[1] += 1;

    /* 2bits: profile */
    data[2] += ((profile << 6) & 0xC0);
    /* 4bits: sampling_frequency_index */
    data[2] += ((m4a_asc->sampling_frequency_index << 2) & 0x3C);
    /* 1bits: private = 0 */
    /* 1bits: channel_configuration */
    data[2] += ((m4a_asc->channels_configuration >> 2) & 0x1);
    /* 2bits: channel_configuration */
    data[3] += ((m4a_asc->channels_configuration << 6) & 0xC0);

    /* 1bits: original */
    /* 1bits: home */
    /* 1bits: copyright_id */
    /* 1bits: copyright_id_start */
    /* 2bits: aac_frame_length */
    data[3] += ((frame_size >> 11) & 0x3);
    /* 8bits: aac_frame_length */
    data[4] += ((frame_size >> 3) & 0xFF);
    /* 3bits: aac_frame_length */
    data[5] += ((frame_size << 5) & 0xE0);

    /* 5bits: adts_buffer_fullness */
    data[5] += ((0x7FF >> 6) & 0x1F);

    /* 6bits: adts_buffer_fullness */
    data[6] += ((0x7FF << 2) & 0x3F);
    /* 2bits: num_raw_data_blocks */

    return M4A_ERROR_OK;
}
