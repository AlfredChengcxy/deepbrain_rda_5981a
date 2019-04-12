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
// Description: unpack m4a file

#include "lightduer_m4a_unpacker.h"
#include <stdlib.h>
#include <string.h>
#include "lightduer_m4a_log.h"
#include "lightduer_m4a_box_parser.h"
#include "lightduer_m4a_util.h"
#include "heap_monitor.h"

#define MEDIA_MALLOC(size)  MALLOC(size, MEDIA)

#define BIG2LITTLE(x) ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) \
            | ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24)

/*
 * if big endian return 0, else return 1.
 */
static uint8_t duer_m4a_is_little_endian(void)
{
    uint32_t x = 1;
    char* p = (char*)&x;
    return *p;
}

int32_t duer_m4a_open_unpacker(duer_m4a_unpacker_t *unpacker)
{
    if (unpacker == NULL || unpacker->stream == NULL || unpacker->stream->read == NULL
            || unpacker->stream->seek == NULL || unpacker->stream->user_data == NULL) {
        LOGE("m4a_open invalid arguments");

        return M4A_ERROR_UNSPECIFIELD;
    }

    duer_m4a_set_position(unpacker, 0);

    return duer_m4a_parse_boxes(unpacker);
}

void duer_m4a_close_unpacker(duer_m4a_unpacker_t *unpacker)
{
    if (unpacker == NULL) {
        return;
    }

    for (int32_t i = 0; i < unpacker->total_tracks; i++) {
        if (unpacker->track[i]) {
            if (unpacker->track[i]->stsz_table) {
                FREE(unpacker->track[i]->stsz_table);
            }

            if (unpacker->track[i]->stts_sample_count) {
                FREE(unpacker->track[i]->stts_sample_count);
            }

            if (unpacker->track[i]->stts_sample_delta) {
                FREE(unpacker->track[i]->stts_sample_delta);
            }

            if (unpacker->track[i]->stsc_first_chunk) {
                FREE(unpacker->track[i]->stsc_first_chunk);
            }

            if (unpacker->track[i]->stsc_samples_per_chunk) {
                FREE(unpacker->track[i]->stsc_samples_per_chunk);
            }

            if (unpacker->track[i]->stsc_sample_desc_index) {
                FREE(unpacker->track[i]->stsc_sample_desc_index);
            }

            if (unpacker->track[i]->stco_chunk_offset) {
                FREE(unpacker->track[i]->stco_chunk_offset);
            }

            if (unpacker->track[i]->decoder_config) {
                FREE(unpacker->track[i]->decoder_config);
            }

            if (unpacker->track[i]->ctts_sample_count) {
                FREE(unpacker->track[i]->ctts_sample_count);
            }

            if (unpacker->track[i]->ctts_sample_offset) {
                FREE(unpacker->track[i]->ctts_sample_offset);
            }

            FREE(unpacker->track[i]);
        }
    }
}

int32_t duer_m4a_get_decoder_config(const duer_m4a_unpacker_t *unpacker, int32_t track,
        uint8_t **buffer, uint32_t *buffer_size)
{
    if (unpacker == NULL || buffer == NULL || buffer_size == NULL) {
        LOGE("invalid arguments");
        return M4A_ERROR_UNSPECIFIELD;
    }

    if (track < 0 || track >= unpacker->total_tracks) {
        *buffer = NULL;
        *buffer_size = 0;
        return M4A_ERROR_UNSPECIFIELD;
    }

    duer_m4a_track_t *ptrack = unpacker->track[track];

    if (ptrack->decoder_config == NULL || ptrack->decoder_config_length == 0) {
        *buffer = NULL;
        *buffer_size = 0;
    } else {
        *buffer = MEDIA_MALLOC(ptrack->decoder_config_length);
        if (*buffer == NULL) {
            *buffer_size = 0;
            LOGE("No free memory");

            return M4A_ERROR_UNSPECIFIELD;
        }

        memcpy(*buffer, ptrack->decoder_config, ptrack->decoder_config_length);
        *buffer_size = ptrack->decoder_config_length;
    }

    return M4A_ERROR_OK;
}

int32_t duer_m4a_get_track_type(const duer_m4a_unpacker_t *unpacker, int32_t track)
{
    if (unpacker == NULL || track < 0 || track >= unpacker->total_tracks) {
        LOGE("invalid arguments");
        return M4A_ERROR_UNSPECIFIELD;
    }

    return unpacker->track[track]->type;
}

int32_t duer_m4a_total_tracks(const duer_m4a_unpacker_t *unpacker)
{
    if (unpacker == NULL) {
        LOGE("invalid arguments");
        return 0;
    }

    return unpacker->total_tracks;
}

int32_t duer_m4a_get_aac_track(const duer_m4a_unpacker_t *unpacker)
{
    if (unpacker == NULL) {
        LOGE("invalid arguments");
        return M4A_ERROR_UNSPECIFIELD;
    }

    int32_t i = 0;
    int32_t numTracks = duer_m4a_total_tracks(unpacker);
    LOGD("numTracks:%d", numTracks);

    for (i = 0; i < numTracks; i++) {
        if (unpacker->track[i]->type == M4A_TRACK_AUDIO) {
            return i;
        }
    }
    LOGE("can't find aac track");

    return M4A_ERROR_UNSPECIFIELD;
}

int32_t duer_m4a_time_scale(const duer_m4a_unpacker_t *unpacker, int32_t track)
{
    if (unpacker == NULL || track < 0 || track >= unpacker->total_tracks) {
        LOGE("invalid arguments");
        return 0;
    }

    return unpacker->track[track]->time_scale;
}

uint32_t duer_m4a_get_avg_bitrate(const duer_m4a_unpacker_t *unpacker, int32_t track)
{
    if (unpacker == NULL || track < 0 || track >= unpacker->total_tracks) {
        LOGE("invalid arguments");
        return 0;
    }

    return unpacker->track[track]->avg_bitrate;
}

uint32_t duer_m4a_get_max_bitrate(const duer_m4a_unpacker_t *unpacker, int32_t track)
{
    if (unpacker == NULL || track < 0 || track >= unpacker->total_tracks) {
        LOGE("invalid arguments");
        return 0;
    }

    return unpacker->track[track]->max_bitrate;
}

int64_t duer_m4a_get_track_duration(const duer_m4a_unpacker_t *unpacker, int32_t track)
{
    if (unpacker == NULL || track < 0 || track >= unpacker->total_tracks) {
        LOGE("invalid arguments");
        return 0;
    }

    return unpacker->track[track]->duration;
}

int32_t duer_m4a_get_samples_number(const duer_m4a_unpacker_t *unpacker, int32_t track)
{
    if (unpacker == NULL || track < 0 || track >= unpacker->total_tracks) {
        LOGE("invalid arguments");
        return 0;
    }

    int32_t i = 0;
    int32_t total = 0;

    for (i = 0; i < unpacker->track[track]->stts_entry_count; i++) {
        total += unpacker->track[track]->stts_sample_count[i];
    }

    return total;
}

uint32_t duer_m4a_get_sample_rate(const duer_m4a_unpacker_t *unpacker, int32_t track)
{
    if (unpacker == NULL || track < 0 || track >= unpacker->total_tracks) {
        LOGE("invalid arguments");
        return 0;
    }

    return unpacker->track[track]->sample_rate;
}

uint32_t duer_m4a_get_channel_count(const duer_m4a_unpacker_t *unpacker, int32_t track)
{
    if (unpacker == NULL || track < 0 || track >= unpacker->total_tracks) {
        LOGE("invalid arguments");
        return 0;
    }

    return unpacker->track[track]->channel_count;
}

uint32_t duer_m4a_get_audio_type(const duer_m4a_unpacker_t *unpacker, int32_t track)
{
    if (unpacker == NULL || track < 0 || track >= unpacker->total_tracks) {
        LOGE("invalid arguments");
        return 0;
    }

    return unpacker->track[track]->audio_type;
}

static int32_t duer_m4a_chunk_of_sample(const duer_m4a_unpacker_t *unpacker,
        int32_t track, int32_t sample)
{
    int32_t i = 0;
    int32_t chunk = 1;
    int32_t next_chunk = 0;
    int32_t dest_chunk = 0;;
    int32_t range_samples = 0;
    int32_t total_samples = 0;
    int32_t samples_per_chunk = 0;
    int32_t chunk_first_sample = 0;
    duer_m4a_track_t *ptrack = unpacker->track[track];

    do {
        next_chunk = ptrack->stsc_first_chunk[i];
        range_samples = (next_chunk - chunk) * samples_per_chunk;

        if (sample < total_samples + range_samples) {
            break;
        }

        samples_per_chunk = ptrack->stsc_samples_per_chunk[i];
        chunk = next_chunk;

        total_samples += range_samples;
        i++;
    } while (i < ptrack->stsc_entry_count);

    if (samples_per_chunk > 0) {
        dest_chunk = (sample - total_samples) / samples_per_chunk + chunk;
    } else {
        dest_chunk = 1;
    }

    chunk_first_sample = total_samples + (dest_chunk - chunk) * samples_per_chunk;

    ptrack->current_chunk = dest_chunk;
    ptrack->current_chunk_first_sample = chunk_first_sample;
    ptrack->current_chunk_samples = samples_per_chunk;

    LOGD("dest_chunk = %d, chunk_first_sample = %d, chunk_samples = %d",
            dest_chunk, chunk_first_sample, samples_per_chunk);
    return M4A_ERROR_OK;
}

static int32_t duer_m4a_chunk_to_offset(duer_m4a_unpacker_t *unpacker,
        int32_t track, int32_t chunk)
{
    duer_m4a_track_t *ptrack = unpacker->track[track];

    if (ptrack->stco_entry_count == 0 || ptrack->stco_chunk_offset == NULL) {
        return 0;
    }

    // the minimum of chunk ID is 1
    chunk--;

    if (chunk < 0) {
        return 0;
    }

    if (chunk >= ptrack->stco_entry_count) {
        chunk = ptrack->stco_entry_count - 1;
    }

    if (chunk < ptrack->stco_first_chunk_loaded
            || chunk >= (ptrack->stco_first_chunk_loaded + MAX_NUM_STCO_LOAD)) {
        // Now need to update ptrack->stco_chunk_offset
        uint32_t i = 0;
        uint32_t num = 0;
        uint64_t last_position = unpacker->current_position;
        uint64_t seek_position = 0;

        ptrack->stco_first_chunk_loaded = chunk - chunk % MAX_NUM_STCO_LOAD;
        num = ptrack->stco_entry_count - ptrack->stco_first_chunk_loaded;
        num = num < MAX_NUM_STCO_LOAD ? num : MAX_NUM_STCO_LOAD;
        seek_position = ptrack->stco_table_offset
                + ptrack->stco_first_chunk_loaded * sizeof(int32_t);

        duer_m4a_set_position(unpacker, seek_position);

        for (i = 0; i < num; i++) {
            ptrack->stco_chunk_offset[i] = duer_m4a_read_int32(unpacker);
        }

        duer_m4a_set_position(unpacker, last_position);
    }

    return ptrack->stco_chunk_offset[chunk - ptrack->stco_first_chunk_loaded];
}

int32_t duer_m4a_fill_stsz_table(duer_m4a_unpacker_t *unpacker, int32_t track, int32_t sample)
{
    if (unpacker == NULL || track < 0 || track >= unpacker->total_tracks || sample < 0) {
        LOGE("invalid arguments");
        return M4A_ERROR_UNSPECIFIELD;
    }

    int32_t ret = 0;
    duer_m4a_track_t *ptrack = unpacker->track[track];

    if (sample > ptrack->current_subchunk_first_sample
            && sample < (ptrack->current_subchunk_first_sample
                + ptrack->current_subchunk_samples)) {
        LOGD("stsz_table has been filled");
        return M4A_ERROR_OK;
    }

    if (sample < ptrack->current_chunk_first_sample
            || sample >= (ptrack->current_chunk_first_sample
                + ptrack->current_chunk_samples)) {
        duer_m4a_chunk_of_sample(unpacker, track, sample);

        ptrack->current_chunk_offset = duer_m4a_chunk_to_offset(
                unpacker, track, ptrack->current_chunk);

        ptrack->current_subchunk_first_sample = ptrack->current_chunk_first_sample;
        ptrack->current_subchunk_samples = 0;
        ptrack->current_subchunk_offset = ptrack->current_chunk_offset;
    } else if (sample < ptrack->current_subchunk_first_sample) {
        ptrack->current_subchunk_first_sample = ptrack->current_chunk_first_sample;
        ptrack->current_subchunk_samples = 0;
        ptrack->current_subchunk_offset = ptrack->current_chunk_offset;
    }

    do {
        if (ptrack->stsz_sample_size == 0) {
            int32_t i = 0;
            int step = 0;
            int num = 0;
            uint32_t result = 0;
            uint32_t last_position = unpacker->current_position;

            if (ptrack->stsz_table == NULL) {
                ptrack->stsz_table = (int32_t *)MEDIA_MALLOC(
                        MAX_SUBCHUNK_SAMPLES * sizeof(int32_t));

                if (ptrack->stsz_table == NULL) {
                    LOGE("Malloc failed, size = %d\n", MAX_SUBCHUNK_SAMPLES * sizeof(int32_t));
                    ret = M4A_ERROR_UNSPECIFIELD;
                    break;
                }
            }

            step = (sample - ptrack->current_subchunk_first_sample
                    - ptrack->current_subchunk_samples) / MAX_SUBCHUNK_SAMPLES + 1;

            while (step > 0) {
                if (ptrack->current_subchunk_samples > 0) {
                    for (i = 0; i < ptrack->current_subchunk_samples; i++) {
                        ptrack->current_subchunk_offset += ptrack->stsz_table[i];
                    }
                    ptrack->current_subchunk_first_sample
                            += ptrack->current_subchunk_samples;
                }

                num = ptrack->current_chunk_first_sample + ptrack->current_chunk_samples
                        - ptrack->current_subchunk_first_sample;
                if (num > MAX_SUBCHUNK_SAMPLES) {
                    num = MAX_SUBCHUNK_SAMPLES;
                }

                unpacker->current_position = ptrack->stsz_table_offset
                        + ptrack->current_subchunk_first_sample * sizeof(int32_t);

                duer_m4a_set_position(unpacker, unpacker->current_position);
                result = duer_m4a_read_data(unpacker, (uint8_t*)ptrack->stsz_table,
                            num * sizeof(int32_t));
                if (result != num * sizeof(int32_t)) {
                    ret = M4A_ERROR_UNSPECIFIELD;
                    break;
                }

                if (duer_m4a_is_little_endian()) {
                    for (i = 0; i < num; i++) {
                        ptrack->stsz_table[i] = BIG2LITTLE(ptrack->stsz_table[i]);
                    }
                }
                ptrack->current_subchunk_samples = num;

                --step;
            }

            if (ret < 0) {
                break;
            }

            duer_m4a_set_position(unpacker, last_position);
        } else {
            ptrack->current_subchunk_samples = ptrack->current_chunk_samples;
        }

        LOGD("chunk:%d, samples:%d, first_sample:%d, offset:%d", ptrack->current_chunk,
                ptrack->current_subchunk_samples, ptrack->current_subchunk_first_sample,
                ptrack->current_subchunk_offset);
        LOGD("stsz_first_sample_size: %d", ptrack->stsz_table[0]);
    } while (0);

    return ret;
}

int32_t duer_m4a_refill_stsz_table(duer_m4a_unpacker_t *unpacker, int32_t track, int32_t sample)
{
    if (unpacker == NULL || track < 0 || track >= unpacker->total_tracks || sample < 0) {
        LOGE("invalid arguments");
        return M4A_ERROR_UNSPECIFIELD;
    }

    int32_t ret = 0;
    int32_t num_samples = duer_m4a_get_samples_number(unpacker, track);
    duer_m4a_track_t *ptrack = unpacker->track[track];

    if ((sample < ptrack->current_subchunk_first_sample
            || sample >= (ptrack->current_subchunk_first_sample
                + ptrack->current_subchunk_samples))
            && (sample < num_samples)) {
        ret = duer_m4a_fill_stsz_table(unpacker, track, sample);
    }

    return ret;
}

static int32_t duer_m4a_sample_range_size(const duer_m4a_unpacker_t *unpacker,
        int32_t track, int32_t chunk_sample, int32_t sample)
{
    int32_t total = 0;
    int32_t num = sample - chunk_sample;
    duer_m4a_track_t *ptrack = unpacker->track[track];

    if (sample >= ptrack->stsz_sample_count || sample < chunk_sample) {
        LOGE("maybe you should fill stsz_table first");
        return 0;
    }

    if (ptrack->stsz_sample_size > 0) {
        total = num * ptrack->stsz_sample_size;
    } else {
        for (int32_t i = 0; i < num; i++) {
            total += ptrack->stsz_table[i];
        }
    }

    return total;
}

static int32_t duer_m4a_audio_frame_size(const duer_m4a_unpacker_t *unpacker,
        int32_t track, int32_t sample)
{
    int32_t size = 0;
    const duer_m4a_track_t * ptrack = unpacker->track[track];
    if (sample < ptrack->current_subchunk_first_sample
            || (sample - ptrack->current_subchunk_first_sample)
                >= ptrack->current_subchunk_samples) {
        LOGE("invalid sample:%d", sample);
        return 0;
    }

    if (ptrack->stsz_sample_size > 0) {
        size = ptrack->stsz_sample_size;
    } else {
        size = ptrack->stsz_table[sample - ptrack->current_subchunk_first_sample];
    }

    return size;
}

int32_t duer_m4a_get_sample_position_and_size(duer_m4a_unpacker_t *unpacker, int32_t track,
        int32_t sample, int32_t *postion, int32_t *size)
{
    if (unpacker == NULL || track < 0 || track >= unpacker->total_tracks
            || postion == NULL || size == NULL) {
        LOGE("invalid arguments");
        return M4A_ERROR_UNSPECIFIELD;
    }

    *postion = unpacker->track[track]->current_subchunk_offset
            + duer_m4a_sample_range_size (unpacker, track,
                unpacker->track[track]->current_subchunk_first_sample, sample);
    *size = duer_m4a_audio_frame_size(unpacker, track, sample);

    return M4A_ERROR_OK;
}
