
#include "baidu_mp3_frame_header_parser.h"
#include <stdio.h>
#include "lightduer_log.h"

typedef struct mpeg_header_s {
    int version;
    int layer;
    int frame_size;
    int samples_per_frame;
    int error_protection;
    int sample_rate;
    int bit_rate;
    int channels;
    int mode;
    int mode_ext;
} mpeg_header_t;

#define MPEG_FRAME_HEADER_SIZE 4
#define MPEG_FRAME_SYNC 0xffe00000
#define MPEG_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#define MPEG_CHANNEL_MODE_STEREO  0
#define MPEG_CHANNEL_MODE_JSTEREO 1
#define MPEG_CHANNEL_MODE_DUAL    2
#define MPEG_CHANNEL_MODE_MONO    3

#define MPEG_RB32(x)                                \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) |    \
    (((const uint8_t*)(x))[1] << 16) |    \
    (((const uint8_t*)(x))[2] <<  8) |    \
    ((const uint8_t*)(x))[3])

static const uint16_t MPEG_FREQ_TAB[3] = { 44100, 48000, 32000 };
static const uint16_t MPEG_BITRATE_TAB[2][3][15] = {
        { {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
        {0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384 },
        {0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320 } },
        { {0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256},
        {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160},
        {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160}
        }
    };

/*
 * 32 bits mpeg frame header format:
 * sync: 11 bit
 * version: 2 bit
 * layer: 2 bit
 * error protection: 1 bit
 * bitrate_index: 4 bit
 * sampling_frequency: 2 bit
 * padding: 1 bit
 * private: 1 bit
 * mode: 2 bit
 * mode extension: 2 bit
 * copyright: 1 bit
 * original: 1 bit
 * emphasis: 2 bit
 */

/* fast header check for resync */
static int check_mpeg_frame_header(uint32_t header) {
    /* header */
    if ((header & MPEG_FRAME_SYNC) != MPEG_FRAME_SYNC) {
        return -1;
    }

    /* layer check */
    if ((header & (3 << 17)) == 0) {
        return -1;
    }

    /* bit rate check*/
    if ((header & (0xf << 12)) == (0xf << 12)) {
        return -1;
    }

    /* sampling frequency check*/
    if ((header & (3 << 10)) == (3 << 10)) {
        return -1;
    }

    return 0;
}

static int mpeg_frame_header_parse(mpeg_header_t *s, uint32_t header) {
    int ret = 0;
    int lsf = 0;
    int mpeg25 = 0;
    int padding = 0;
    int bitrate = 0;
    int frame_size = 0;
    int sample_rate = 0;
    int bitrate_index = 0;
    int sample_rate_index = 0;

    ret = check_mpeg_frame_header(header);
    if (ret < 0) {
        goto EXIT;
    }

    /* 00-MPEG 2.5, 01-undefine, 10-MPEG 2, 11-MPEG 1 */
    if (header & (1 << 20)) {
        lsf = (header & (1 << 19)) ? 0 : 1;
        mpeg25 = 0;
    } else {
        lsf = 1;
        mpeg25 = 1;
    }

    s->version = lsf + mpeg25 + 1;
    s->layer = 4 - ((header >> 17) & 3);
    /* extract frequency */
    sample_rate_index = (header >> 10) & 3;
    if (sample_rate_index >= MPEG_ARRAY_ELEMS(MPEG_FREQ_TAB)) {
        sample_rate_index = 0;
    }
    sample_rate = MPEG_FREQ_TAB[sample_rate_index] >> (lsf + mpeg25);
    sample_rate_index += 3 * (lsf + mpeg25);
    s->error_protection = ((header >> 16) & 1) ^ 1;
    s->sample_rate = sample_rate;

    bitrate_index = (header >> 12) & 0xf;
    padding = (header >> 9) & 1;
    //extension = (header >> 8) & 1;
    s->mode = (header >> 6) & 3;
    s->mode_ext = (header >> 4) & 3;
    //copyright = (header >> 3) & 1;
    //original = (header >> 2) & 1;
    //emphasis = header & 3;

    if (s->mode == MPEG_CHANNEL_MODE_MONO) {
        s->channels = 1;
    } else {
        s->channels = 2;
    }

    if (bitrate_index != 0) {
        bitrate = MPEG_BITRATE_TAB[lsf][s->layer - 1][bitrate_index];
        s->bit_rate = bitrate * 1000;
        switch(s->layer) {
        case 1:
            frame_size = (bitrate * 12000) / sample_rate;
            frame_size = (frame_size + padding) * 4;
            break;
        case 2:
            frame_size = (bitrate * 144000) / sample_rate;
            frame_size += padding;
            break;
        case 3:
            frame_size = (bitrate * 144000) / (sample_rate << lsf);
            frame_size += padding;
            break;
        default:
            ret = -1;
            goto EXIT;
        }
        s->frame_size = frame_size;
    } else {
        /* if no frame size computed, signal it */
        ret = 1;
        goto EXIT;
    }

    // output samples
    switch(s->layer) {
    case 1:
        s->samples_per_frame = 384;
        break;
    case 2:
        s->samples_per_frame = 1152;
        break;
    case 3:
        if (lsf){
            s->samples_per_frame = 576;
        } else {
            s->samples_per_frame = 1152;
        }
        break;
    default:
        ret = -1;
        break;
    }

EXIT:
    return ret;
}

static int get_mpeg_info(const uint8_t *buf, uint32_t size, mpeg_header_t *h) {
    if (buf == NULL || size < 4 || h == NULL) {
        return -1;
    }

    int ret = 0;
    uint32_t header = 0;
    const uint8_t *end = buf + size - sizeof(header) + 1;

    /* skip zero data */
    while (buf < end && !*buf) {
        buf++;
    }

    for(; buf < end; ++buf) {
        header = MPEG_RB32(buf);
        ret = mpeg_frame_header_parse(h, header);
        if (ret == 0) {
            return 0;
        }
    }

    return -1;
}

int get_mp3_frame_bitrate(const uint8_t *buf, uint32_t size) {
    int ret = 0;
    int bitrate = 0;
    mpeg_header_t header;
    if (buf == NULL || size < 4) {
        goto EXIT;
    }

    ret = get_mpeg_info(buf, size, &header);
    if (ret < 0) {
        DUER_LOGW("cant't get mpeg info");
    } else {
        bitrate = header.bit_rate;
        DUER_LOGD("mp3 frame version:%d layer:%d bitrate:%d frame_size:%d samples_per_frame:%d"
            " sample_rate:%d, channels:%d", header.version, header.layer, header.bit_rate,
            header.frame_size, header.samples_per_frame, header.sample_rate, header.channels);
    }

EXIT:
    return bitrate;
}
