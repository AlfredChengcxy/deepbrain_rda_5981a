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
// Description: unpack m4a file, external API

#ifndef BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_UNPACKER_H
#define BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_UNPACKER_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define M4A_ERROR_OK                 0
#define M4A_ERROR_UNSPECIFIELD      -1
#define M4A_ERROR_NO_MEMORY         -2

#define MAX_TRACKS           5      /* The maximum track number included in m4a file */
#define MAX_SUBCHUNK_SAMPLES 100    /* The maximum sample number of each subchunk */
#define MAX_NUM_STCO_LOAD    128    /* The maximum number of loading stco table to memory */

/*
 * type of m4a track
 */
typedef enum {
    M4A_TRACK_UNKNOWN,
    M4A_TRACK_AUDIO,
    M4A_TRACK_VIDEO,
    M4A_TRACK_SYSTEM,
} duer_m4a_track_type_t;

/*
 * file callback structure
 */
typedef struct duer_m4a_file_callback_s{
    uint32_t (*read)(void *user_data, void *buffer, uint32_t length);
    uint32_t (*seek)(void *user_data, uint64_t position);
    void *user_data;
} duer_m4a_file_callback_t;

/*
 * the informations of track structure
 */
typedef struct duer_m4a_track_s{
    duer_m4a_track_type_t type;

    /* stsd */
    int32_t     stsd_entry_count;

    /* mp4a */
    int32_t     channel_count;
    int32_t     sample_size;
    uint16_t    sample_rate;

    /* stsz */
    int32_t     stsz_sample_size;
    int32_t     stsz_sample_count;
    int32_t     *stsz_table;
    int32_t     stsz_table_offset;  /* the offset of the start of stsz_table into the file */

    /* stts */
    int32_t     stts_entry_count;
    int32_t     *stts_sample_count;
    int32_t     *stts_sample_delta;

    /* stsc */
    int32_t     stsc_entry_count;
    int32_t     *stsc_first_chunk;
    int32_t     *stsc_samples_per_chunk;
    int32_t     *stsc_sample_desc_index;

    /* stco */
    int32_t     stco_entry_count;
    int32_t     *stco_chunk_offset;
    int32_t     stco_table_offset;  /* the offset of the start of stco_table into the file */
    int32_t     stco_first_chunk_loaded;

    /* ctts */
    int32_t     ctts_entry_count;
    int32_t     *ctts_sample_count;
    int32_t     *ctts_sample_offset;

    /* esde */
    int32_t     audio_type;
    uint8_t     *decoder_config;
    int32_t     decoder_config_length;
    uint32_t    max_bitrate;
    uint32_t    avg_bitrate;

    /* mdhd */
    uint32_t    time_scale;
    uint64_t    duration;

    /* for refill stsz_table */
    int32_t     current_chunk;  //Current chunk ID, the minimum is 1
    int32_t     current_chunk_first_sample;
    int32_t     current_chunk_samples;
    int32_t     current_chunk_offset;
    int32_t     current_subchunk_first_sample;
    int32_t     current_subchunk_samples;
    int32_t     current_subchunk_offset;
} duer_m4a_track_t;

/*
 * main context of m4a unpacker
 */
typedef struct duer_m4a_unpacker_s{
    /* stream to read from */
    duer_m4a_file_callback_t *stream;
    int64_t current_position;

    /* mvhd */
    int32_t time_scale;
    int32_t duration;

    /* incremental track index while reading the file */
    int32_t total_tracks;

    /* track data */
    duer_m4a_track_t *track[MAX_TRACKS];
} duer_m4a_unpacker_t;

/*
 * Open m4a unpacker and parse m4a box
 *
 * @Param unpacker, m4a unpacker context
 * @Return success: 0, fail: < 0
 */
int32_t duer_m4a_open_unpacker(duer_m4a_unpacker_t *unpacker);

/*
 * close m4a unpacker
 *
 * @Param unpacker, m4a unpacker context
 */
void duer_m4a_close_unpacker(duer_m4a_unpacker_t *unpacker);

/*
 * Get the index of aac track
 *
 * @Param unpacker, m4a unpacker context
 * @Return the index of aac track
 */
int32_t duer_m4a_get_aac_track(const duer_m4a_unpacker_t *unpacker);

/*
 * Get the decoder config of track
 *
 * @Param unpacker, m4a unpacker context
 * @Param track, track index
 * @Param buffer, out buffer stored decoder config, should freed by caller
 * @Param buffer_size, point to buffer size
 * @Return success: 0, fail: < 0
 */
int32_t duer_m4a_get_decoder_config(const duer_m4a_unpacker_t *unpacker, int32_t track,
        uint8_t **buffer, uint32_t *buffer_size);

/*
 * Get the total number of samples included in specified track
 *
 * @Param unpacker, m4a unpacker context
 * @Param track, track index
 * @Return the number of samples
 */
int32_t duer_m4a_get_samples_number(const duer_m4a_unpacker_t *unpacker, int32_t track);

/*
 * Fill specified track's stsz table, only fill one subchunck per time
 * One subchunck has MAX_SUBCHUNK_SAMPLES samples at most
 *
 * @Param unpacker, m4a unpacker context
 * @Param track, track index
 * @Param sample, sample id, stsz table should include this sample after fill
 * @Return success: 0, fail: < 0
 */
int32_t duer_m4a_fill_stsz_table(duer_m4a_unpacker_t *unpacker, int32_t track, int32_t sample);

/*
 * Refill specified track's stsz table, if sample id changed
 *
 * @Param unpacker, m4a unpacker context
 * @Param track, track index
 * @Param sample, sample id, stsz table should include this sample after fill
 * @Return success: 0, fail: < 0
 */
int32_t duer_m4a_refill_stsz_table(duer_m4a_unpacker_t *unpacker, int32_t track, int32_t sample);

/*
 * Get the postion in file and size of specified sample
 *
 * @Param unpacker, m4a unpacker context
 * @Param track, track index
 * @Param sample, sample id
 * @Param postion, output the postion
 * @Param size, output the size
 * @Return success: 0, fail: < 0
 */
int32_t duer_m4a_get_sample_position_and_size(duer_m4a_unpacker_t *unpacker, int32_t track,
        int32_t sample, int32_t *postion, int32_t *size);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_LIGHTDUER_AUDIO_DECODER_M4A_UNPACKER_H
