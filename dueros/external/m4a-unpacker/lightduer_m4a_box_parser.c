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
// Description: parse common m4a(mp4) boxes

#include "lightduer_m4a_box_parser.h"
#include <string.h>
#include "lightduer_m4a_util.h"
#include "lightduer_m4a_log.h"
#include "heap_monitor.h"

/* boxes with subboxes */
#define M4A_BOX_MOOV    1
#define M4A_BOX_TRAK    2
#define M4A_BOX_EDTS    3
#define M4A_BOX_MDIA    4
#define M4A_BOX_MINF    5
#define M4A_BOX_STBL    6
#define M4A_BOX_UDTA    7
#define M4A_BOX_ILST    8 /* iTunes Metadata list */
#define M4A_BOX_TITLE   9
#define M4A_BOX_ARTIST  10
#define M4A_BOX_WRITER  11
#define M4A_BOX_ALBUM   12
#define M4A_BOX_DATE    13
#define M4A_BOX_TOOL    14
#define M4A_BOX_COMMENT 15
#define M4A_BOX_GENRE1  16
#define M4A_BOX_TRACK   17
#define M4A_BOX_DISC    18
#define M4A_BOX_COMPILATION 19
#define M4A_BOX_GENRE2  20
#define M4A_BOX_TEMPO   21
#define M4A_BOX_COVER   22
#define M4A_BOX_DRMS    23
#define M4A_BOX_SINF    24
#define M4A_BOX_SCHI    25

#define M4A_SUBBOX      128

/* boxes without subboxes */
#define M4A_BOX_FTYP    129
#define M4A_BOX_MDAT    130
#define M4A_BOX_MVHD    131
#define M4A_BOX_TKHD    132
#define M4A_BOX_TREF    133
#define M4A_BOX_MDHD    134
#define M4A_BOX_VMHD    135
#define M4A_BOX_SMHD    136
#define M4A_BOX_HMHD    137
#define M4A_BOX_STSD    138
#define M4A_BOX_STTS    139
#define M4A_BOX_STSZ    140
#define M4A_BOX_STZ2    141
#define M4A_BOX_STCO    142
#define M4A_BOX_STSC    143
#define M4A_BOX_MP4A    144
#define M4A_BOX_MP4V    145
#define M4A_BOX_MP4S    146
#define M4A_BOX_ESDS    147
#define M4A_BOX_META    148 /* iTunes Metadata box */
#define M4A_BOX_NAME    149 /* iTunes Metadata name box */
#define M4A_BOX_DATA    150 /* iTunes Metadata data box */
#define M4A_BOX_CTTS    151
#define M4A_BOX_FRMA    152
#define M4A_BOX_IVIV    153
#define M4A_BOX_PRIV    154
#define M4A_BOX_USER    155
#define M4A_BOX_KEY     156

#define M4A_BOX_ALBUM_ARTIST    157
#define M4A_BOX_CONTENTGROUP    158
#define M4A_BOX_LYRICS          159
#define M4A_BOX_DESCRIPTION     160
#define M4A_BOX_NETWORK         161
#define M4A_BOX_SHOW            162
#define M4A_BOX_EPISODENAME     163
#define M4A_BOX_SORTTITLE       164
#define M4A_BOX_SORTALBUM       165
#define M4A_BOX_SORTARTIST      166
#define M4A_BOX_SORTALBUMARTIST 167
#define M4A_BOX_SORTWRITER      168
#define M4A_BOX_SORTSHOW        169
#define M4A_BOX_SEASON          170
#define M4A_BOX_EPISODE         171
#define M4A_BOX_PODCAST         172

#define M4A_BOX_UNKNOWN         255
#define M4A_BOX_SKIP M4A_BOX_UNKNOWN
#define M4A_BOX_FREE M4A_BOX_UNKNOWN

#define MEDIA_MALLOC(size)  MALLOC(size, MEDIA)

static const uint8_t COPYRIGHT_SYMBOL = 0xA9;
static const uint32_t BOX_HEADER_LENGTH = 8;
static const uint8_t ES_DESCR_TAG = 0x03;
static const uint8_t DECODER_CONFIG_DESCR_TAG = 0x04;
static const uint8_t DEC_SPECIFIC_DESCR_TAG = 0x05;
static const uint32_t MIN_ES_DESCR_LENGTH = 20;
static const uint32_t MIN_DECODER_CONFIG_DESCR_LENGTH = 13;

/* parse box header size */
static uint32_t duer_m4a_box_get_size(const uint8_t *data)
{
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

/* compare 2 box names, returns 1 for equal, 0 for unequal */
static int32_t duer_m4a_box_compare(
        uint8_t a1, uint8_t b1, uint8_t c1, uint8_t d1,
        uint8_t a2, uint8_t b2, uint8_t c2, uint8_t d2)
{
    if (a1 == a2 && b1 == b2 && c1 == c2 && d1 == d2) {
        return 1;
    } else {
        return 0;
    }
}

static uint8_t duer_m4a_box_name_to_type(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    if (a == 'm') {
        if (duer_m4a_box_compare(a, b, c, d, 'm','o','o','v')) {
            return M4A_BOX_MOOV;
        } else if (duer_m4a_box_compare(a, b, c, d, 'm','i','n','f')) {
            return M4A_BOX_MINF;
        } else if (duer_m4a_box_compare(a, b, c, d, 'm','d','i','a')) {
            return M4A_BOX_MDIA;
        } else if (duer_m4a_box_compare(a, b, c, d, 'm','d','a','t')) {
            return M4A_BOX_MDAT;
        } else if (duer_m4a_box_compare(a, b, c, d, 'm','d','h','d')) {
            return M4A_BOX_MDHD;
        } else if (duer_m4a_box_compare(a, b, c, d, 'm','v','h','d')) {
            return M4A_BOX_MVHD;
        } else if (duer_m4a_box_compare(a, b, c, d, 'm','p','4','a')) {
            return M4A_BOX_MP4A;
        } else if (duer_m4a_box_compare(a, b, c, d, 'm','p','4','v')) {
            return M4A_BOX_MP4V;
        } else if (duer_m4a_box_compare(a, b, c, d, 'm','p','4','s')) {
            return M4A_BOX_MP4S;
        } else if (duer_m4a_box_compare(a, b, c, d, 'm','e','t','a')) {
            return M4A_BOX_META;
        } else {
            return M4A_BOX_UNKNOWN;
        }
    } else if (a == 't') {
        if (duer_m4a_box_compare(a, b, c, d, 't','r','a','k')) {
            return M4A_BOX_TRAK;
        } else if (duer_m4a_box_compare(a, b, c, d, 't','k','h','d')) {
            return M4A_BOX_TKHD;
        } else if (duer_m4a_box_compare(a, b, c, d, 't','r','e','f')) {
            return M4A_BOX_TREF;
        } else if (duer_m4a_box_compare(a, b, c, d, 't','r','k','n')) {
            return M4A_BOX_TRACK;
        } else if (duer_m4a_box_compare(a, b, c, d, 't','m','p','o')) {
            return M4A_BOX_TEMPO;
        } else if (duer_m4a_box_compare(a, b, c, d, 't','v','n','n')) {
            return M4A_BOX_NETWORK;
        } else if (duer_m4a_box_compare(a, b, c, d, 't','v','s','h')) {
            return M4A_BOX_SHOW;
        } else if (duer_m4a_box_compare(a, b, c, d, 't','v','e','n')) {
            return M4A_BOX_EPISODENAME;
        } else if (duer_m4a_box_compare(a, b, c, d, 't','v','s','n')) {
            return M4A_BOX_SEASON;
        } else if (duer_m4a_box_compare(a, b, c, d, 't','v','e','s')) {
            return M4A_BOX_EPISODE;
        } else {
            return M4A_BOX_UNKNOWN;
        }
    } else if (a == 's') {
        if (duer_m4a_box_compare(a, b, c, d, 's','t','b','l')) {
            return M4A_BOX_STBL;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','m','h','d')) {
            return M4A_BOX_SMHD;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','t','s','d')) {
            return M4A_BOX_STSD;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','t','t','s')) {
            return M4A_BOX_STTS;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','t','c','o')) {
            return M4A_BOX_STCO;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','t','s','c')) {
            return M4A_BOX_STSC;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','t','s','z')) {
            return M4A_BOX_STSZ;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','t','z','2')) {
            return M4A_BOX_STZ2;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','k','i','p')) {
            return M4A_BOX_SKIP;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','i','n','f')) {
            return M4A_BOX_SINF;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','c','h','i')) {
            return M4A_BOX_SCHI;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','o','n','m')) {
            return M4A_BOX_SORTTITLE;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','o','a','l')) {
            return M4A_BOX_SORTALBUM;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','o','a','r')) {
            return M4A_BOX_SORTARTIST;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','o','a','a')) {
            return M4A_BOX_SORTALBUMARTIST;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','o','c','o')) {
            return M4A_BOX_SORTWRITER;
        } else if (duer_m4a_box_compare(a, b, c, d, 's','o','s','n')) {
            return M4A_BOX_SORTSHOW;
        } else {
            return M4A_BOX_UNKNOWN;
        }
    } else if (a == COPYRIGHT_SYMBOL) {
        if (duer_m4a_box_compare(a, b, c, d, COPYRIGHT_SYMBOL,'n','a','m')) {
            return M4A_BOX_TITLE;
        } else if (duer_m4a_box_compare(a, b, c, d, COPYRIGHT_SYMBOL,'A','R','T')) {
            return M4A_BOX_ARTIST;
        } else if (duer_m4a_box_compare(a, b, c, d, COPYRIGHT_SYMBOL,'w','r','t')) {
            return M4A_BOX_WRITER;
        } else if (duer_m4a_box_compare(a, b, c, d, COPYRIGHT_SYMBOL,'a','l','b')) {
            return M4A_BOX_ALBUM;
        } else if (duer_m4a_box_compare(a, b, c, d, COPYRIGHT_SYMBOL,'d','a','y')) {
            return M4A_BOX_DATE;
        } else if (duer_m4a_box_compare(a, b, c, d, COPYRIGHT_SYMBOL,'t','o','o')) {
            return M4A_BOX_TOOL;
        } else if (duer_m4a_box_compare(a, b, c, d, COPYRIGHT_SYMBOL,'c','m','t')) {
            return M4A_BOX_COMMENT;
        } else if (duer_m4a_box_compare(a, b, c, d, COPYRIGHT_SYMBOL,'g','e','n')) {
            return M4A_BOX_GENRE1;
        } else if (duer_m4a_box_compare(a, b, c, d, COPYRIGHT_SYMBOL,'g','r','p')) {
            return M4A_BOX_CONTENTGROUP;
        } else if (duer_m4a_box_compare(a, b, c, d, COPYRIGHT_SYMBOL,'l','y','r')) {
            return M4A_BOX_LYRICS;
        } else {
            return M4A_BOX_UNKNOWN;
        }
    } else {
        if (duer_m4a_box_compare(a, b, c, d, 'e','d','t','s')) {
            return M4A_BOX_EDTS;
        } else if (duer_m4a_box_compare(a, b, c, d, 'e','s','d','s')) {
            return M4A_BOX_ESDS;
        } else if (duer_m4a_box_compare(a, b, c, d, 'f','t','y','p')) {
            return M4A_BOX_FTYP;
        } else if (duer_m4a_box_compare(a, b, c, d, 'f','r','e','e')) {
            return M4A_BOX_FREE;
        } else if (duer_m4a_box_compare(a, b, c, d, 'h','m','h','d')) {
            return M4A_BOX_HMHD;
        } else if (duer_m4a_box_compare(a, b, c, d, 'v','m','h','d')) {
            return M4A_BOX_VMHD;
        } else if (duer_m4a_box_compare(a, b, c, d, 'u','d','t','a')) {
            return M4A_BOX_UDTA;
        } else if (duer_m4a_box_compare(a, b, c, d, 'i','l','s','t')) {
            return M4A_BOX_ILST;
        } else if (duer_m4a_box_compare(a, b, c, d, 'n','a','m','e')) {
            return M4A_BOX_NAME;
        } else if (duer_m4a_box_compare(a, b, c, d, 'd','a','t','a')) {
            return M4A_BOX_DATA;
        } else if (duer_m4a_box_compare(a, b, c, d, 'd','i','s','k')) {
            return M4A_BOX_DISC;
        } else if (duer_m4a_box_compare(a, b, c, d, 'g','n','r','e')) {
            return M4A_BOX_GENRE2;
        } else if (duer_m4a_box_compare(a, b, c, d, 'c','o','v','r')) {
            return M4A_BOX_COVER;
        } else if (duer_m4a_box_compare(a, b, c, d, 'c','p','i','l')) {
            return M4A_BOX_COMPILATION;
        } else if (duer_m4a_box_compare(a, b, c, d, 'c','t','t','s')) {
            return M4A_BOX_CTTS;
        } else if (duer_m4a_box_compare(a, b, c, d, 'd','r','m','s')) {
            return M4A_BOX_DRMS;
        } else if (duer_m4a_box_compare(a, b, c, d, 'f','r','m','a')) {
            return M4A_BOX_FRMA;
        } else if (duer_m4a_box_compare(a, b, c, d, 'p','r','i','v')) {
            return M4A_BOX_PRIV;
        } else if (duer_m4a_box_compare(a, b, c, d, 'i','v','i','v')) {
            return M4A_BOX_IVIV;
        } else if (duer_m4a_box_compare(a, b, c, d, 'u','s','e','r')) {
            return M4A_BOX_USER;
        } else if (duer_m4a_box_compare(a, b, c, d, 'k','e','y',' ')) {
            return M4A_BOX_KEY;
        } else if (duer_m4a_box_compare(a, b, c, d, 'a','A','R','T')) {
            return M4A_BOX_ALBUM_ARTIST;
        } else if (duer_m4a_box_compare(a, b, c, d, 'd','e','s','c')) {
            return M4A_BOX_DESCRIPTION;
        } else if (duer_m4a_box_compare(a, b, c, d, 'p','c','s','t')) {
            return M4A_BOX_PODCAST;
        } else {
            return M4A_BOX_UNKNOWN;
        }
    }
}

/* read box header, return box size, box size is with header included */
static uint64_t duer_m4a_box_read_header(duer_m4a_unpacker_t *unpacker,
        uint8_t *box_type, uint8_t *header_size)
{
    uint64_t size = 0;
    int32_t ret = 0;
    uint8_t box_header[BOX_HEADER_LENGTH];

    ret = duer_m4a_read_data(unpacker, box_header, BOX_HEADER_LENGTH);
    if (ret != BOX_HEADER_LENGTH) {
        return 0;
    }

    size = duer_m4a_box_get_size(box_header);
    *header_size = BOX_HEADER_LENGTH;

    /* check for 64 bit box size */
    if (size == 1) {
        *header_size += 8;
        size = duer_m4a_read_int64(unpacker);
    }

    LOGD("read box: %c%c%c%c\n", box_header[4], box_header[5], box_header[6], box_header[7]);

    *box_type = duer_m4a_box_name_to_type(box_header[4], box_header[5],
            box_header[6], box_header[7]);

    return size;
}

static int32_t duer_m4a_read_stsz(duer_m4a_unpacker_t *unpacker)
{
    duer_m4a_track_t *track = unpacker->track[unpacker->total_tracks - 1];

    // skip version
    duer_m4a_read_byte(unpacker);
    // skip flags
    duer_m4a_read_int24(unpacker);

    track->stsz_sample_size = duer_m4a_read_int32(unpacker);
    track->stsz_sample_count = duer_m4a_read_int32(unpacker);

    if (track->stsz_sample_size == 0) {
        track->stsz_table_offset = duer_m4a_position(unpacker);

        // memory is not enough, we will read part of stsz_table when used
#if 0
        track->stsz_table = (int32_t*)MEDIA_MALLOC(track->stsz_sample_count * sizeof(int32_t));

        for (int32_t i = 0; i < track->stsz_sample_count; i++) {
            track->stsz_table[i] = duer_m4a_read_int32(unpacker);
        }
#else
        uint32_t stsz_table_size = track->stsz_sample_count * sizeof(int32_t);
        duer_m4a_set_position(unpacker, duer_m4a_position(unpacker) + stsz_table_size);
#endif
    }

    return M4A_ERROR_OK;
}

static int32_t duer_m4a_read_esds(duer_m4a_unpacker_t *unpacker)
{
    uint8_t tag = 0;
    int32_t ret = 0;
    duer_m4a_track_t *track = unpacker->track[unpacker->total_tracks - 1];

    // skip version
    duer_m4a_read_byte(unpacker);
    // skip flags
    duer_m4a_read_int24(unpacker);

    /* get and verify ES_DescrTag */
    tag = duer_m4a_read_byte(unpacker);

    do {
        /* read length */
        if (duer_m4a_read_m4a_descr_length(unpacker) < MIN_ES_DESCR_LENGTH) {
            ret = M4A_ERROR_UNSPECIFIELD;
            break;
        }

        if (tag == ES_DESCR_TAG) {
            /* skip ID and priority */
            duer_m4a_read_int24(unpacker);
        } else {
            /* skip ID */
            duer_m4a_read_int16(unpacker);
        }

        /* get and verify DecoderConfigDescrTag */
        if (duer_m4a_read_byte(unpacker) != DECODER_CONFIG_DESCR_TAG) {
            ret = M4A_ERROR_UNSPECIFIELD;
            break;
        }

        /* read length */
        if (duer_m4a_read_m4a_descr_length(unpacker) < MIN_DECODER_CONFIG_DESCR_LENGTH) {
            ret = M4A_ERROR_UNSPECIFIELD;
            break;
        }

        track->audio_type = duer_m4a_read_byte(unpacker);
        /* skip stream type */
        duer_m4a_read_int32(unpacker);
        track->max_bitrate = duer_m4a_read_int32(unpacker);
        track->avg_bitrate = duer_m4a_read_int32(unpacker);

        /* get and verify DecSpecificInfoTag */
        if (duer_m4a_read_byte(unpacker) != DEC_SPECIFIC_DESCR_TAG) {
            ret = M4A_ERROR_UNSPECIFIELD;
            break;
        }

        /* read length */
        track->decoder_config_length = duer_m4a_read_m4a_descr_length(unpacker);

        if (track->decoder_config) {
            FREE(track->decoder_config);
        }

        track->decoder_config = MEDIA_MALLOC(track->decoder_config_length);
        if (track->decoder_config) {
            duer_m4a_read_data(unpacker, track->decoder_config, track->decoder_config_length);
        } else {
            track->decoder_config_length = 0;
            LOGE("No free memory.");
            ret = M4A_ERROR_NO_MEMORY;
        }
    } while (0);

    /* will skip the remainder of the box */
    return ret;
}

static int32_t duer_m4a_read_mp4a(duer_m4a_unpacker_t *unpacker)
{
    int32_t i = 0;
    uint8_t box_type = 0;
    uint8_t header_size = 0;
    int32_t ret = 0;
    duer_m4a_track_t *track = unpacker->track[unpacker->total_tracks - 1];

    /* skip reserved */
    for (i = 0; i < 6; i++) {
        duer_m4a_read_byte(unpacker);
    }
    /* skip data_reference_index */
    duer_m4a_read_int16(unpacker);
    /* skip reserved */
    duer_m4a_read_int32(unpacker);
    /* skip reserved */
    duer_m4a_read_int32(unpacker);

    track->channel_count = duer_m4a_read_int16(unpacker);
    track->sample_size = duer_m4a_read_int16(unpacker);

    /* skip pre_defined */
    duer_m4a_read_int16(unpacker);
    /* skip reserved */
    duer_m4a_read_int16(unpacker);

    track->sample_rate = duer_m4a_read_int16(unpacker);

    /* skip reserved */
    duer_m4a_read_int16(unpacker);

    duer_m4a_box_read_header(unpacker, &box_type, &header_size);
    if (box_type == M4A_BOX_ESDS) {
        ret = duer_m4a_read_esds(unpacker);
    }
    LOGD("channel_count %d, sample_size = %d, sample_rate = %d",
            track->channel_count, track->sample_size, track->sample_rate);

    return ret;
}

static int32_t duer_m4a_read_stsd(duer_m4a_unpacker_t *unpacker)
{
    int32_t i = 0;
    uint8_t header_size = 0;
    int32_t ret = 0;
    duer_m4a_track_t *track = unpacker->track[unpacker->total_tracks - 1];

    // skip version
    duer_m4a_read_byte(unpacker);
    // skip flags
    duer_m4a_read_int24(unpacker);

    track->stsd_entry_count = duer_m4a_read_int32(unpacker);

    for (i = 0; i < track->stsd_entry_count; i++) {
        uint64_t skip = duer_m4a_position(unpacker);
        uint8_t box_type = 0;
        uint64_t size = duer_m4a_box_read_header(unpacker, &box_type, &header_size);
        skip += size;

        if (box_type == M4A_BOX_MP4A) {
            track->type = M4A_TRACK_AUDIO;
            ret = duer_m4a_read_mp4a(unpacker);
            if (ret < 0) {
                break;
            }
        } else if (box_type == M4A_BOX_MP4V) {
            track->type = M4A_TRACK_VIDEO;
        } else if (box_type == M4A_BOX_MP4S) {
            track->type = M4A_TRACK_SYSTEM;
        } else {
            track->type = M4A_TRACK_UNKNOWN;
        }

        duer_m4a_set_position(unpacker, skip);
    }

    return ret;
}

static int32_t duer_m4a_read_stsc(duer_m4a_unpacker_t *unpacker)
{
    int32_t i = 0;
    duer_m4a_track_t *track = unpacker->track[unpacker->total_tracks - 1];

    // skip version
    duer_m4a_read_byte(unpacker);
    // skip flags
    duer_m4a_read_int24(unpacker);

    track->stsc_entry_count = duer_m4a_read_int32(unpacker);

    track->stsc_first_chunk =
            (int32_t*)MEDIA_MALLOC(track->stsc_entry_count * sizeof(int32_t));
    track->stsc_samples_per_chunk =
            (int32_t*)MEDIA_MALLOC(track->stsc_entry_count * sizeof(int32_t));
    track->stsc_sample_desc_index =
            (int32_t*)MEDIA_MALLOC(track->stsc_entry_count * sizeof(int32_t));

    if (track->stsc_first_chunk == NULL
            || track->stsc_samples_per_chunk == NULL
            || track->stsc_sample_desc_index == NULL) {
        if (track->stsc_first_chunk) {
            FREE(track->stsc_first_chunk);
            track->stsc_first_chunk = NULL;
        }

        if (track->stsc_samples_per_chunk) {
            FREE(track->stsc_samples_per_chunk);
            track->stsc_samples_per_chunk = NULL;
        }

        if (track->stsc_sample_desc_index) {
            FREE(track->stsc_sample_desc_index);
            track->stsc_sample_desc_index = NULL;
        }
        track->stsc_entry_count = 0;
        LOGE("No free memory.");

        return M4A_ERROR_NO_MEMORY;
    } else {
        for (i = 0; i < track->stsc_entry_count; i++) {
            track->stsc_first_chunk[i] = duer_m4a_read_int32(unpacker);
            track->stsc_samples_per_chunk[i] = duer_m4a_read_int32(unpacker);
            track->stsc_sample_desc_index[i] = duer_m4a_read_int32(unpacker);
            LOGD("stsc_entry[%d]:%d, %d, %d", i, track->stsc_first_chunk[i],
                track->stsc_samples_per_chunk[i], track->stsc_sample_desc_index[i]);
        }

        return M4A_ERROR_OK;
    }
}

static int32_t duer_m4a_read_stco(duer_m4a_unpacker_t *unpacker)
{
    int32_t i = 0;
    int32_t num = 0;
    duer_m4a_track_t *track = unpacker->track[unpacker->total_tracks - 1];

    // skip version
    duer_m4a_read_byte(unpacker);
    // skip flags
    duer_m4a_read_int24(unpacker);

    track->stco_entry_count = duer_m4a_read_int32(unpacker);

    /*
     * if stco_entry_count is bigger than MAX_NUM_STCO_LOAD,
     * we will read part of stco_table when used
     */
    num = track->stco_entry_count > MAX_NUM_STCO_LOAD
            ? MAX_NUM_STCO_LOAD : track->stco_entry_count;
    track->stco_chunk_offset = (int32_t*)MEDIA_MALLOC(num * sizeof(int32_t));

    if (track->stco_chunk_offset == NULL) {
        track->stco_entry_count = 0;
        LOGE("No free memory.");
        return M4A_ERROR_NO_MEMORY;
    } else {
        track->stco_table_offset = duer_m4a_position(unpacker);
        track->stco_first_chunk_loaded = 0;

        for (i = 0; i < num; i++) {
            track->stco_chunk_offset[i] = duer_m4a_read_int32(unpacker);
        }

        // skip left stco tables
        if (num < track->stco_entry_count) {
            duer_m4a_set_position(unpacker,
                track->stco_table_offset + track->stco_entry_count * sizeof(int32_t));
        }

        return M4A_ERROR_OK;
    }
}

static int32_t duer_m4a_read_ctts(duer_m4a_unpacker_t *unpacker)
{
    int32_t i = 0;
    duer_m4a_track_t *track = unpacker->track[unpacker->total_tracks - 1];

    if (track->ctts_entry_count > 0) {
        return M4A_ERROR_OK;
    }

    // skip version
    duer_m4a_read_byte(unpacker);
    // skip flags
    duer_m4a_read_int24(unpacker);

    track->ctts_entry_count = duer_m4a_read_int32(unpacker);

    track->ctts_sample_count = (int32_t*)MEDIA_MALLOC(track->ctts_entry_count * sizeof(int32_t));
    track->ctts_sample_offset = (int32_t*)MEDIA_MALLOC(track->ctts_entry_count * sizeof(int32_t));

    if (track->ctts_sample_count == NULL || track->ctts_sample_offset == NULL) {
        if (track->ctts_sample_count) {
            FREE(track->ctts_sample_count);
            track->ctts_sample_count = NULL;
        }

        if (track->ctts_sample_offset) {
            FREE(track->ctts_sample_offset);
            track->ctts_sample_offset = NULL;
        }
        track->ctts_entry_count = 0;
        LOGE("No free memory.");

        return M4A_ERROR_NO_MEMORY;
    } else {
        for (i = 0; i < track->ctts_entry_count; i++) {
            track->ctts_sample_count[i] = duer_m4a_read_int32(unpacker);
            track->ctts_sample_offset[i] = duer_m4a_read_int32(unpacker);
        }

        return M4A_ERROR_OK;
    }
}

static int32_t duer_m4a_read_stts(duer_m4a_unpacker_t *unpacker)
{
    int32_t i = 0;
    duer_m4a_track_t *track = unpacker->track[unpacker->total_tracks - 1];

    if (track->stts_entry_count > 0) {
        return M4A_ERROR_OK;
    }

    // skip version
    duer_m4a_read_byte(unpacker);
    // skip flags
    duer_m4a_read_int24(unpacker);

    track->stts_entry_count = duer_m4a_read_int32(unpacker);

    track->stts_sample_count = (int32_t*)MEDIA_MALLOC(track->stts_entry_count * sizeof(int32_t));
    track->stts_sample_delta = (int32_t*)MEDIA_MALLOC(track->stts_entry_count * sizeof(int32_t));

    if (track->stts_sample_count == NULL || track->stts_sample_delta == NULL) {
        if (track->stts_sample_count) {
            FREE(track->stts_sample_count);
            track->stts_sample_count = NULL;
        }

        if (track->stts_sample_delta) {
            FREE(track->stts_sample_delta);
            track->stts_sample_delta = NULL;
        }
        track->stts_entry_count = 0;
        LOGE("No free memory.");

        return M4A_ERROR_UNSPECIFIELD;
    } else {
        for (i = 0; i < track->stts_entry_count; i++) {
            track->stts_sample_count[i] = duer_m4a_read_int32(unpacker);
            track->stts_sample_delta[i] = duer_m4a_read_int32(unpacker);
        }
        return M4A_ERROR_OK;
    }
}

static int32_t duer_m4a_read_mvhd(duer_m4a_unpacker_t *unpacker)
{
    int32_t i = 0;

    // skip version
    duer_m4a_read_byte(unpacker);
    // skip flags
    duer_m4a_read_int24(unpacker);
    /* skip creation_time */
    duer_m4a_read_int32(unpacker);
    /* skip modification_time */
    duer_m4a_read_int32(unpacker);

    unpacker->time_scale = duer_m4a_read_int32(unpacker);
    unpacker->duration = duer_m4a_read_int32(unpacker);

    /* skip preferred_rate */
    duer_m4a_read_int32(unpacker);
    /* skip preferred_volume */
    duer_m4a_read_int16(unpacker);

    /* skip 10 bytes reserved */
    for (i = 0; i < 10; i++) {
        duer_m4a_read_byte(unpacker);
    }

    /* skip matrix */
    for (i = 0; i < 9; i++) {
        duer_m4a_read_int32(unpacker);
    }

    /* skip preview_time */
    duer_m4a_read_int32(unpacker);
    /* skip preview_duration */
    duer_m4a_read_int32(unpacker);
    /* skip poster_time */
    duer_m4a_read_int32(unpacker);
    /* skip selection_time */
    duer_m4a_read_int32(unpacker);
    /* skip selection_duration */
    duer_m4a_read_int32(unpacker);
    /* skip current_time */
    duer_m4a_read_int32(unpacker);
    /* skip next_track_id */
    duer_m4a_read_int32(unpacker);

    return M4A_ERROR_OK;
}

static int32_t duer_m4a_read_mdhd(duer_m4a_unpacker_t *unpacker)
{
    uint32_t version = 0;
    duer_m4a_track_t *track = unpacker->track[unpacker->total_tracks - 1];

    version = duer_m4a_read_int32(unpacker);
    if (version == 0) {
        uint32_t duration;

        /* skip creation-time */
        duer_m4a_read_int32(unpacker);
        /* skip modification-time */
        duer_m4a_read_int32(unpacker);
        track->time_scale = duer_m4a_read_int32(unpacker);
        duration = duer_m4a_read_int32(unpacker);
        track->duration = (duration == (uint32_t)(-1)) ? (uint64_t)(-1) : (uint64_t)(duration);
    } else { // version == 1
        /* skip creation-time */
        duer_m4a_read_int64(unpacker);
        /* skip modification-time */
        duer_m4a_read_int64(unpacker);
        track->time_scale = duer_m4a_read_int32(unpacker);
        track->duration = duer_m4a_read_int64(unpacker);
    }

    /* skip language */
    duer_m4a_read_int16(unpacker);
    /* skip pre-defined */
    duer_m4a_read_int16(unpacker);

    return M4A_ERROR_OK;
}

static int32_t duer_m4a_box_read(duer_m4a_unpacker_t *unpacker, uint32_t size, uint8_t box_type)
{
    uint64_t dest_position = duer_m4a_position(unpacker) + size - 8;
    int32_t ret = 0;

    if (box_type == M4A_BOX_STSZ) {
        /* sample size box */
        ret = duer_m4a_read_stsz(unpacker);
    } else if (box_type == M4A_BOX_STTS) {
        /* time to sample box */
        ret = duer_m4a_read_stts(unpacker);
    } else if (box_type == M4A_BOX_CTTS) {
        /* composition offset box */
        ret = duer_m4a_read_ctts(unpacker);
    } else if (box_type == M4A_BOX_STSC) {
        /* sample to chunk box */
        ret = duer_m4a_read_stsc(unpacker);
    } else if (box_type == M4A_BOX_STCO) {
        /* chunk offset box */
        ret = duer_m4a_read_stco(unpacker);
    } else if (box_type == M4A_BOX_STSD) {
        /* sample description box */
        ret = duer_m4a_read_stsd(unpacker);
    } else if (box_type == M4A_BOX_MVHD) {
        /* movie header box */
        ret = duer_m4a_read_mvhd(unpacker);
    } else if (box_type == M4A_BOX_MDHD) {
        /* track header */
        ret = duer_m4a_read_mdhd(unpacker);
    }

    duer_m4a_set_position(unpacker, dest_position);

    return ret;
}

static int32_t duer_m4a_add_track(duer_m4a_unpacker_t *unpacker)
{
    if (unpacker->total_tracks == MAX_TRACKS) {
        LOGE("num of tracks is more than MAX_TRACKS(%d)", MAX_TRACKS);
        return M4A_ERROR_UNSPECIFIELD;
    }

    unpacker->track[unpacker->total_tracks] = MEDIA_MALLOC(sizeof(duer_m4a_track_t));
    if (unpacker->track[unpacker->total_tracks] == NULL) {
        LOGE("No free memory");
        return M4A_ERROR_NO_MEMORY;
    }

    memset(unpacker->track[unpacker->total_tracks], 0, sizeof(duer_m4a_track_t));

    unpacker->total_tracks++;

    return M4A_ERROR_OK;
}

/* parse boxes that are sub boxes of other boxes */
static int32_t duer_m4a_parse_sub_boxes(duer_m4a_unpacker_t *unpacker, uint64_t total_size)
{
    uint64_t size = 0;
    uint8_t box_type = 0;
    uint64_t sum_size = 0;
    uint8_t header_size = 0;
    int32_t ret = 0;

    while (sum_size < total_size) {
        size = duer_m4a_box_read_header(unpacker, &box_type, &header_size);
        sum_size += size;

        /* check for end of file */
        if (size == 0) {
            break;
        }

        /* we're starting to read a new track, update index,
         * so that all data and tables get written in the right place
         */
        if (box_type == M4A_BOX_TRAK) {
            ret = duer_m4a_add_track(unpacker);
            if (ret < 0) {
                break;
            }
        }

        /* parse subboxes */
        if (box_type < M4A_SUBBOX) {
            ret = duer_m4a_parse_sub_boxes(unpacker, size - header_size);
        } else {
            ret = duer_m4a_box_read(unpacker, (uint32_t)size, box_type);
        }

        if (ret < 0) {
            break;
        }
    }

    return ret;
}

/* parse root boxes */
int32_t duer_m4a_parse_boxes(duer_m4a_unpacker_t *unpacker)
{
    if (unpacker == NULL) {
        return M4A_ERROR_UNSPECIFIELD;
    }

    uint64_t size = 0;
    uint8_t box_type = 0;
    uint8_t header_size = 0;
    int32_t ret = 0;

    while ((size = duer_m4a_box_read_header(unpacker, &box_type, &header_size)) != 0) {
        /* parse subboxes */
        if (box_type < M4A_SUBBOX) {
            ret = duer_m4a_parse_sub_boxes(unpacker, size-header_size);
        } else {
            /* skip this box */
            ret = duer_m4a_set_position(unpacker, duer_m4a_position(unpacker) + size - header_size);
        }

        if (ret < 0) {
            break;
        }
    }

    return ret;
}
