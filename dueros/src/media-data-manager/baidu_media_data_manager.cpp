// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Media data Manager

#include "baidu_media_data_manager.h"
#include "mbed.h"
#include "DirHandle.h"
#include "baidu_media_play.h"
#include "baidu_time_calculate.h"
#include "baidu_media_play_m4a.h"
#include "baidu_media_type.h"
#include "lightduer_log.h"
#include "heap_monitor.h"
#include "baidu_measure_stack.h"
#include "baidu_media_play_progress.h"
#include "YTManage.h"
#include "lightduer_http_client.h"
#include "lightduer_http_client_ops.h"
#include "change_voice.h"


bool bExitMagicData = false;
bool bExitMagicDatav1 = false;

namespace duer {

#define MEDIA_MALLOC(size)  MALLOC(size, MEDIA)
static int const HTTP_PERSISTENT_CONNECT_TIME = 60000; // keep the http connect 60 s

enum MediaSourceType {
    MEDIA_TYPE_URL = 1,
    MEDIA_TYPE_FILE_PATH,
    MEDIA_TYPE_DATA,
    MEDIA_TYPE_MAGIC_VOICE,
};

struct MdmMessage {
    //media url or file path buffer  need to be free by receiver
    char*           media_url_or_path;
    int             flags;
    int             position;
	char *			armnb_data;
    const char*     data;       // used by MEDIA_TYPE_DATA only
    size_t          data_size;  // used by MEDIA_TYPE_DATA only
    MediaSourceType data_type;
    MediaType       media_type;
};

struct PreviousMedia {
    char*           url;        // if url is null, PreviousMedia is invalid
    MediaSourceType data_type;
    MediaType       media_type;
    int             flags;
    int             write_size; // size of data has writted to codec
    char*           header;     // stored header of media file, only for .wav now
    int             header_size;
};

struct DownloadContext {
    bool is_local;
    bool is_continue_play;
    int  flags;
};

/*
 * when a play message been sended, set it to s_mdm_msg
 * mdm_thead will handle s_mdm_msg
 */
static MdmMessage* s_mdm_msg = NULL;
static rtos::Mutex s_mdm_msg_mutex;
static rtos::Semaphore s_mdm_msg_sem(0);

static volatile int s_mdm_stop_flag = 0;
// static Mutex s_mdm_stop_mutex;

static const uint32_t MDM_THREAD_STACK_SIZE = 1024 * 6;
static Thread s_mdm_thread(osPriorityHigh, MDM_THREAD_STACK_SIZE);

static MediaType s_media_type = TYPE_NULL;
static duer_http_client_t *s_http_client = NULL;
static bool s_unsupport_format = false;

// The below variables are used to continue play previous media file
static PreviousMedia s_previous_media;
static bool s_previous_media_paused = false;

static bool s_stop_completely = false;

static int s_media_seek_position = -1;
static mdm_media_play_failed_cb s_play_failed_cb;

static void reset_previous_data() {
    media_play_clear_previous_m4a();

    if (s_previous_media.header != NULL) {
        FREE(s_previous_media.header);
        s_previous_media.header = NULL;
    }

    if (s_previous_media.url != NULL) {
        FREE(s_previous_media.url);
        s_previous_media.url = NULL;
    }

    s_previous_media.write_size = 0;
    s_previous_media_paused = false;
}

static int save_wav_header(const char* buf, size_t len) {
    if (s_previous_media.header != NULL) {
        FREE(s_previous_media.header);
        s_previous_media.header = NULL;
    }
    s_previous_media.header_size = 0;

    // size of wav header is 44 bytes at least
    if (buf == NULL || len < 44) {
        return -1;
    }

    // wav header must start with "RIFF"
    if (buf[0] != 'R' || buf[1] != 'I' || buf[2] != 'F' || buf[3] != 'F') {
        DUER_LOGW("Invalid wav header!");
        return -1;
    }

    // "data" flag won't appear before 0x24
    for (int i = 0x24; i < len - 3; i++) {
        // wav header end with 4 bytes "data" flag and 4 bytes size of data
        if (buf[i] == 'd' && buf[i + 1] == 'a' && buf[i + 2] == 't' && buf[i + 3] == 'a') {
            s_previous_media.header_size = i + 8;
            break;
        }
    }

    if (s_previous_media.header_size == 0) {
        DUER_LOGW("Wav head can't find data flag!");
        return -1;
    }

    DUER_LOGD("Size of wav header is %d", s_previous_media.header_size);
    s_previous_media.header = (char*)MEDIA_MALLOC(s_previous_media.header_size);
    if (s_previous_media.header == NULL) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("No free memory!");
        return -1;
    }
    memcpy(s_previous_media.header, buf, s_previous_media.header_size);

    return 0;
}

static bool is_support_wav_format(const char* header, size_t len) {
    // size of wav header is 44 bytes at least
    if (header == NULL || len < 44) {
        return false;
    }

    // wav header must start with "RIFF"
    if (header[0] != 'R' || header[1] != 'I' || header[2] != 'F' || header[3] != 'F') {
        DUER_LOGW("Invalid wav header!");
        return false;
    }

    short code_id = header[20] | (header[21] << 8);

    switch (code_id) {
    case 0x01:   // PCM
    case 0x11:   // ADPCM
    case 0x06:   // ALAW
    case 0x07:   // ULAW
    case 0x55:   // MP3
    case 0xff:   // AAC
    case 0xa106: // AAC_MS
    case 0x1601: // AAC_2
    case 0x1602: // AAC_LATM
    case 0x706D: // AVCODEC_AAC
        return true;
    default:
        DUER_LOGD("Unsupport wav code_id = 0x%x", code_id);
        return false;
    };
}

static bool is_support_m4a_format(const char* header, size_t len) {
    if (header == NULL) {
        return false;
    }

    if ('f' != header[4] || 't' != header[5] || 'y' != header[6] || 'p' != header[7]) {
        DUER_LOGW("Invalid m4a header!");
        return false;
    }

    int box_size = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];

    if ('m' != header[box_size + 4] || 'o' != header[box_size + 5]
            || 'o' != header[box_size + 6] || 'v' != header[box_size + 7]) {
        DUER_LOGW("moov box is not at header!");
        return false;
    }

    int header_size = box_size + (header[box_size] << 24) | (header[box_size + 1] << 16) |
                    (header[box_size + 2] << 8) | header[box_size + 3];
    if (header_size > MediaPlayM4A::MAX_M4A_HEADER_SIZE) {
        DUER_LOGW("head size(%d) of m4a file is bigger than limit", header_size);
        return false;
    }

    return true;
}

static bool is_support_format(const char* header, size_t len, MediaType type) {
    switch (type) {
    case TYPE_MP3:
    case TYPE_AAC:
    case TYPE_SPEECH:
    case TYPE_TS:
    case TYPE_AMR:
        return true;
    case TYPE_WAV:
        return is_support_wav_format(header, len);
    case TYPE_M4A:
        return is_support_m4a_format(header, len);
    default:
        DUER_LOGE("Unsupport media type:%d!", type);
    }

    return false;
}

/*
 * refer to the ID3V2 standard for details
 */
static bool mp3_id3v2_match(const char* buf) {
    if (buf == NULL) {
        return false;
    }

    return  buf[0] == 'I' &&
            buf[1] == 'D' &&
            buf[2] == '3' &&
            buf[3] != 0xff &&
            buf[4] != 0xff &&
            (buf[6] & 0x80) == 0 &&
            (buf[7] & 0x80) == 0 &&
            (buf[8] & 0x80) == 0 &&
            (buf[9] & 0x80) == 0;
}

static int mp3_id3v2_header_len(const char* buf) {
    if (buf == NULL) {
        return 0;
    }

    const int ID3V2_HEADER_SIZE = 10;
    int len = ((buf[6] & 0x7f) << 21) + ((buf[7] & 0x7f) << 14) +
        ((buf[8] & 0x7f) << 7) + (buf[9] & 0x7f) + ID3V2_HEADER_SIZE;

    if ((buf[3] == 0x3 || buf[3] == 0x4) && (buf[5] & 0x40)) {
        len += ID3V2_HEADER_SIZE;
    }

    return len;
}

/**
 * FUNC:
 * int mdm_media_data_out_handler(void* client, duer_http_data_pos_t pos, char* buf, size_t len, char* type)
 *
 * DESC:
 * callback for output media data, here user can receive data
 * and put it into media player's data buffer
 *
 * PARAM:
 * @param[in] p_user_ctx: usr ctx registed by user
 * @param[in] pos: to identify if it is data stream's start, or middle , or end of data stream
 * @param[in] buf: buffer stored media data
 * @param[in] len: data length in 'buf'
 * @param[in] type: data type to identify media or others
 */
static int mdm_media_data_out_handler(void* p_user_ctx, duer_http_data_pos_t pos, const char* buf,
        size_t len, const char* type) {
    if (p_user_ctx == NULL) {
        DUER_LOGE("p_user_ctx is null");
        return -1;
    }

    if (len > FRAME_SIZE) {
        DUER_LOGE("buffer length is bigger than FRAME_SIZE");
        return 0;
    }

    if (media_play_error()) {
        mdm_notify_to_stop();
        return -1;
    }

    DownloadContext* ctx = (DownloadContext*)p_user_ctx;
    static MediaType media_type = TYPE_NULL;
    static int skip_len = -1; // for skip ID3V2 header of mp3
    MediaPlayerMessage msg;
    switch (pos) {
    case DUER_HTTP_DATA_FIRST: {
        if (ctx->is_local == false) {
            if (buf == NULL || len < 1) {
                DUER_LOGE("source file has no data!");
                return -1;
            }
        }

        if (!ctx->is_continue_play && !is_support_format(buf, len, s_media_type)) {
            mdm_notify_to_stop();
            s_unsupport_format = true;
            return -1;
        }

        if (ctx->is_continue_play) {
            ctx->flags |= MEDIA_FLAG_CONTINUE_PLAY;
        }

        skip_len = -1;
        media_type = (s_media_type == TYPE_SPEECH) ? TYPE_MP3 : s_media_type;
        msg.cmd = PLAYER_CMD_START;
        if (media_type == TYPE_WAV) {
            if (ctx->is_continue_play) {
                if (s_previous_media.header != NULL) {
                    msg.size = s_previous_media.header_size;
                    msg.type = media_type;
                    g_media_play_buffer->write(&msg, sizeof(msg));
                    g_media_play_buffer->write(
                            s_previous_media.header, s_previous_media.header_size);
                    msg.cmd = PLAYER_CMD_CONTINUE;
                    DUER_LOGD("write saved wav header");
                } else {
                    DUER_LOGE("s_previous_media.header is NULL!");
                }
            } else {
                save_wav_header(buf, len);
            }
        } else if (media_type == TYPE_MP3) {
            const int MIN_ID3V2_LENGTH = 10;
            if (len > MIN_ID3V2_LENGTH && mp3_id3v2_match(buf)) {
                skip_len = mp3_id3v2_header_len(buf);
                DUER_LOGD("mp3 has ID3V2 header, length is %d", skip_len);
            }
        }
        break;
    }
    case DUER_HTTP_DATA_MID: {
        if (buf == NULL || len < 1) {
            DUER_LOGW("Data output handler DUER_HTTP_DATA_MID buf is NULL!");
            return -1;
        }
        msg.cmd = PLAYER_CMD_CONTINUE;
        break;
    }
    case DUER_HTTP_DATA_LAST: {
        msg.cmd = PLAYER_CMD_STOP;
        break;
    }
    default:
        DUER_LOGE("Data output handler error pos!");
        return -1;
    }

    /*
     * skip ID3V2 header
     * when skip_len == 0, all data of ID3V2 header has been skipped,
     * set cmd of current msg to PLAYER_CMD_START.
     */
    if (skip_len > -1) {
        if (skip_len >= len) {
            skip_len -= len;
            return 0;
        } else {
            msg.cmd = PLAYER_CMD_START;
            len -= skip_len;
            buf += skip_len;
            skip_len = -1;
        }
    }

    msg.size = len;
    msg.type = media_type; //mdm_media_type_map(type);
    msg.flags = ctx->flags;

    DUER_LOGV("Data output handler send pos:%d, %d bytes data to media player", pos, msg.size);

    g_media_play_buffer->write(&msg, sizeof(msg));
    g_media_play_buffer->write(buf, len);

    return 0;
}

/**
 * Translate string to lower case, returned value should be free by caller
 */
static char* to_lowercase(const char* str) {
    if (str == NULL) {
        return NULL;
    }

    char* lower = (char*)MEDIA_MALLOC(strlen(str) + 1);
    if (lower == NULL) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("No memory!");
        return NULL;
    }
    int i = 0;
    for (; str[i] != '\0'; ++i) {
        lower[i] = str[i];
        if (lower[i] >= 'A' && lower[i] <= 'Z') {
            lower[i] += 32;
        }
    }
    lower[i] = '\0';
    return lower;
}

static MediaType determine_media_type_by_suffix(const char* str, bool is_url) {
    MediaType type = TYPE_NULL;
    char* lower = to_lowercase(str);
    if (strstr(lower, ".mp3") != NULL) {
        type = TYPE_MP3;
    } else if (strstr(lower, ".m4a") != NULL) {
        type = TYPE_M4A;
    } else if (strstr(lower, ".aac") != NULL) {
        type = TYPE_AAC;
    } else if (strstr(lower, ".wav") != NULL) {
        type = TYPE_WAV;
    } else if (strstr(lower, ".ts") != NULL) {
        type = TYPE_TS;
    } else if (strstr(lower, ".m3u8") != NULL) {
        type = TYPE_HLS;
    } else if (strstr(lower, ".amr") != NULL) {
        type = TYPE_AMR;
    } else if (is_url) {
        //type = TYPE_MP3;
        type = TYPE_AMR;
    } else {
        type = TYPE_NULL;
    }
    FREE(lower);
    lower = NULL;

    return type;
}

static void set_redirection_url(const char* url) {
    DUER_LOGD("set_redirection_url %s", url);
    if (s_media_type != TYPE_SPEECH) {
        s_media_type = determine_media_type_by_suffix(url, true);
    }
}

static int adjust_position(int position) {
    if (position <= 0) {
        return 0;
    }

    if (s_media_type == TYPE_M4A) {
        return media_play_seek_m4a_file(position);
    } else if (s_media_type == TYPE_WAV) {
        return position < s_previous_media.header_size ? s_previous_media.header_size : position;
    } else {
        return position;
    }
}

/**
 * FUNC:
 * static int mdm_get_data_by_media_url(char* media_url, int position, int flags)
 *
 * DESC:
 * get media data from media url and send it to media player's message queue
 *
 * PARAM:
 * @param[in] media_url: media url
 */
static int mdm_get_data_by_media_url(const char* media_url, int position, int flags) {
    DownloadContext ctx = { false, false, flags };
    static duer_http_client_t *speech_http_client = NULL;
    static duer_http_client_t *audio_http_client = NULL;

    if (position > 0) {
        ctx.is_continue_play = true;
        DUER_LOGD("position of url data is %d", position);
    }

    mdm_reset_stop_flag();

    if (s_media_type == TYPE_SPEECH) {
        if (!speech_http_client) {
            speech_http_client = duer_create_http_client();
            if (!speech_http_client) {
                //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
                DUER_LOGE("Memory overlow");
                return -1;
            }
        }
        s_http_client = speech_http_client;
    } else {
        if (!audio_http_client) {
            audio_http_client = duer_create_http_client();
            if (!audio_http_client) {
                //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
                DUER_LOGE("Memory overlow");
                return -1;
            }
        }
        s_http_client = audio_http_client;
    }
    duer_http_reg_data_hdlr(s_http_client, mdm_media_data_out_handler, &ctx);
    duer_http_reg_stop_notify_cb(s_http_client, mdm_check_need_to_stop);
    duer_http_reg_url_get_cb(s_http_client, set_redirection_url);

    int ret = duer_http_get(s_http_client, media_url, position, HTTP_PERSISTENT_CONNECT_TIME);
    s_http_client = NULL;

    if (s_unsupport_format) {
        DUER_LOGE("Unsupport media format: %s", media_url);
    }

    if (media_play_error()) {
        if (flags & MEDIA_FLAG_DCS_URL) {
        }
        DUER_LOGE("media play error: %s", media_url);
        return -1;
    }

    return mdm_check_need_to_stop() == 1 ? 1 : ret;
}

/**
 * FUNC:
 * int mdm_get_data_by_media_file_path(const char* media_file_path)
 *
 * DESC:
 * get media data from media file and send it to media player's message queue
 *
 * PARAM:
 * @param[in] media_file_path: media file path
 */
static int mdm_get_data_by_media_file_path(
        const char* media_file_path, const char* type, int position, int flags) {
    mdm_reset_stop_flag();

    if (media_file_path == NULL) {
        DUER_LOGE("media file path is NULL!");
        return -1;
    }

    FILE* media_file = fopen(media_file_path, "r");
    if (media_file == NULL) {
        DUER_LOGE("open file failed!");
        return -1;
    }

    DownloadContext ctx = { true, false, flags };
    if (position > 0) {
        if (fseek(media_file, position, SEEK_SET) != 0) {
            DUER_LOGE("seek file failed!");
            fclose(media_file);
            return -1;
        }
        DUER_LOGD("Seek file %d", position);
        ctx.is_continue_play = true;
    } else {
        ctx.is_continue_play = false;
    }

    int read_bytes = 0;
    char read_buff[FRAME_SIZE];
    duer_http_data_pos_t data_pos = DUER_HTTP_DATA_FIRST;
    bool is_start = true;
    do {
        if (mdm_check_need_to_stop()) {
            DUER_LOGD("Stopped reading media file data by stop flag!");
            PERFORM_STATISTIC_BEGIN(PERFORM_STATISTIC_PLAY_LOCAL_MEDIA, MAX_TIME_CALCULATE_COUNT);
            mdm_media_data_out_handler(&ctx, DUER_HTTP_DATA_LAST, NULL, 0, type);
            PERFORM_STATISTIC_END(PERFORM_STATISTIC_PLAY_LOCAL_MEDIA, 0,
                    UNCONTINUOUS_TIME_MEASURE);
            break;
        }
        PERFORM_STATISTIC_BEGIN(PERFORM_STATISTIC_GET_MEDIA_BY_PATH, MAX_TIME_CALCULATE_COUNT);
        read_bytes = fread(read_buff, 1, FRAME_SIZE, media_file);
        PERFORM_STATISTIC_END(PERFORM_STATISTIC_GET_MEDIA_BY_PATH,
                              read_bytes,
                              UNCONTINUOUS_TIME_MEASURE);
        DUER_LOGV("Reading %d bytes for %d bytes", read_bytes, FRAME_SIZE);
        if (is_start) {
            is_start = false;
            data_pos = DUER_HTTP_DATA_FIRST;
        } else {
            data_pos = DUER_HTTP_DATA_MID;
            if (read_bytes < FRAME_SIZE) {
                DUER_LOGD("Finished transforing file!");
                data_pos = DUER_HTTP_DATA_LAST;
                PERFORM_STATISTIC_BEGIN(PERFORM_STATISTIC_PLAY_LOCAL_MEDIA,
                    MAX_TIME_CALCULATE_COUNT);
                mdm_media_data_out_handler(&ctx, data_pos, read_buff, read_bytes, type);
                PERFORM_STATISTIC_END(PERFORM_STATISTIC_PLAY_LOCAL_MEDIA,
                                      read_bytes,
                                      UNCONTINUOUS_TIME_MEASURE);
                break;
            }
        }
        PERFORM_STATISTIC_BEGIN(PERFORM_STATISTIC_PLAY_LOCAL_MEDIA, MAX_TIME_CALCULATE_COUNT);
        mdm_media_data_out_handler(&ctx, data_pos, read_buff, read_bytes, type);
        PERFORM_STATISTIC_END(PERFORM_STATISTIC_PLAY_LOCAL_MEDIA,
                              read_bytes,
                              UNCONTINUOUS_TIME_MEASURE);
    } while (read_bytes > 0);

    fclose(media_file);

    if (s_unsupport_format) {
        DUER_LOGW("Unsupport media format: %s", media_file_path);
    }

    if (media_play_error()) {
        DUER_LOGE("media play error: %s", media_file_path);
        return -1;
    }

    return data_pos == DUER_HTTP_DATA_LAST ? 0 : 1;
}

static int mdm_send_data_to_buffer(const char* data, size_t size, int flags) {
    mdm_reset_stop_flag();

    int pos = 0;
    int write_size = 0;
    duer_http_data_pos_t data_pos = DUER_HTTP_DATA_FIRST;
    DownloadContext ctx = { true, false, flags };
    while (size > 0) {
        if (pos == 0) {
            data_pos = DUER_HTTP_DATA_FIRST;
            write_size = FRAME_SIZE > size ? size : FRAME_SIZE;
        } else if (size <= FRAME_SIZE) {
            data_pos = DUER_HTTP_DATA_LAST;
            write_size = size;
        } else {
            data_pos = DUER_HTTP_DATA_MID;
            write_size = FRAME_SIZE;
        }

        mdm_media_data_out_handler(&ctx, data_pos, data + pos, write_size, NULL);

        if (mdm_check_need_to_stop()) {
            if (data_pos != DUER_HTTP_DATA_LAST) {
                mdm_media_data_out_handler(&ctx, DUER_HTTP_DATA_LAST, NULL, 0, NULL);
            }
            break;
        }

        pos += write_size;
        size -= write_size;
    }

    return 0;
}

static char g_read_buff[FRAME_SIZE];

static int mdm_send_magic_data_to_buffer(char* data, size_t size, int flags)
{
    mdm_reset_stop_flag();
	
    DownloadContext ctx = { true, false, flags };

    int read_bytes = 0;
    char *read_buff = g_read_buff;

    int buff_bytes = 0;
	char *buff = NULL;
	
    unsigned int copy_bytes = 0;
	int ret = 0;
	int i = 0;
    duer_http_data_pos_t data_pos = DUER_HTTP_DATA_FIRST;
    bool is_start = true;

	if(yt_voice_change_init(data, size) < 0) {
		DUER_LOGE("yt_voice_change_init error");
		return -1;
	}

	yt_voice_set_wave_head(read_buff);
	read_bytes += 44;			

#if 1/// add by lijun

	//int nPreVolume = duer::YTMediaManager::instance().get_volume();
	//duer::YTMediaManager::instance().set_volume(100);	
#endif

	//bExitMagicData = false;
	bExitMagicDatav1 = false;
    while(1) 
	{
        if (mdm_check_need_to_stop()) {
            DUER_LOGD("Stopped reading media file data by stop flag!");
            mdm_media_data_out_handler(&ctx, DUER_HTTP_DATA_LAST, NULL, 0, NULL);
            break;
        }

		ret = yt_voice_change_get_data(&buff, &buff_bytes); 		
		DUER_LOGD("Reading %d bytes for %d bytes ret %d", buff_bytes, FRAME_SIZE,ret);
		
		if(read_bytes + buff_bytes < FRAME_SIZE)
		{
			memcpy(read_buff + read_bytes, buff, buff_bytes);
			read_bytes += buff_bytes;			
		}
		else {
			if(is_start) {
				is_start = false;
				data_pos = DUER_HTTP_DATA_FIRST;
			}	
			else {
				data_pos = DUER_HTTP_DATA_MID;
			}
			DUER_LOGD("read_bytes:%d",read_bytes);
			
			copy_bytes = FRAME_SIZE - read_bytes;
			memcpy(read_buff + read_bytes, buff, copy_bytes);
			mdm_media_data_out_handler(&ctx, data_pos, read_buff, FRAME_SIZE, NULL);

			
			DUER_LOGD("send bytes:%d",copy_bytes + read_bytes);

		//// by lijun a big bug for lj_magic
			if(buff_bytes - copy_bytes > FRAME_SIZE)
			{
				memcpy(read_buff, buff + copy_bytes, FRAME_SIZE);
				mdm_media_data_out_handler(&ctx, data_pos, read_buff, FRAME_SIZE, NULL);
				read_bytes = buff_bytes - FRAME_SIZE;
			}

			else
			{
				memcpy(read_buff, buff + copy_bytes, buff_bytes - copy_bytes);
				read_bytes = buff_bytes - copy_bytes;
			}
					
			//DUER_LOGI("remain bytes:%d ret:%d",read_bytes , ret);
		}


		if(ret == 0) 
			{
			data_pos = DUER_HTTP_DATA_LAST;			
			DUER_LOGD("remain bytes:%d  buff_bytes%d ret:%d",read_bytes, buff_bytes, ret);
			if(read_bytes + buff_bytes < FRAME_SIZE)
			{
					if((read_bytes + buff_bytes) > 44)//TERENCE---2019/03/11: bug fix
						{
							memcpy(read_buff + read_bytes, buff, buff_bytes);
							read_bytes += buff_bytes;	

							
							mdm_media_data_out_handler(&ctx, data_pos, read_buff, read_bytes, NULL);
						}
			}
			else {

				
				copy_bytes = FRAME_SIZE - read_bytes;


				//TERENCE---2019/03/11: bug fix
				if(buff_bytes >= copy_bytes)
					{
						memcpy(read_buff + read_bytes, buff, copy_bytes);
						mdm_media_data_out_handler(&ctx, data_pos, read_buff, FRAME_SIZE, NULL);


						memcpy(read_buff, buff + copy_bytes, buff_bytes - copy_bytes);
						read_bytes = buff_bytes - copy_bytes;			
						mdm_media_data_out_handler(&ctx, data_pos, read_buff, read_bytes, NULL);
					}
					else
					{						
						memcpy(read_buff + read_bytes, buff, buff_bytes);
						for(i=(read_bytes+buff_bytes); i<FRAME_SIZE;i++)read_buff[i] = 0;
						mdm_media_data_out_handler(&ctx, data_pos, read_buff, FRAME_SIZE, NULL);
					}
					
					//TERENCE---2019/03/11: bug fix
			}

			break;

			
		}	


		
    }

	yt_voice_change_free();
	//bExitMagicData = true;
	bExitMagicDatav1 = true;
	
#if 1
	//duer::YTMediaManager::instance().set_volume(nPreVolume);	
#endif
	return data_pos == DUER_HTTP_DATA_LAST ? 0 : 1;
}

static int mdm_send_msg_to_mdm_thread(MdmMessage* msg) {
    s_mdm_msg_mutex.lock();
    if (s_mdm_msg != NULL) {
        if (s_mdm_msg->media_url_or_path != NULL) {
            FREE(s_mdm_msg->media_url_or_path);
            s_mdm_msg->media_url_or_path = NULL;
        }
        FREE(s_mdm_msg);
    }
    s_mdm_msg = msg;
    s_mdm_msg_mutex.unlock();
    s_mdm_msg_sem.release();

    return 0;
}

/**
 * FUNC:
 * void mdm_send_media_url(const char* media_url, int flags)
 *
 * DESC:
 * external  interface to send media url mdm message to MDM thread Queue
 *
 * PARAM:
 * @param[in] media_url: media url
 */
void mdm_send_media_url(const char* media_url, int flags) {
    mdm_notify_to_stop();

    if (media_url == NULL) {
        DUER_LOGE("media url is NULL!");
        return;
    }

    MediaType media_type = determine_media_type_by_suffix(media_url, true);
    if (flags & MEDIA_FLAG_SPEECH) {
        if (media_type != TYPE_MP3) {
            DUER_LOGW("speech's media type is not mp3?");
        }
        media_type = TYPE_SPEECH;
    } else {
        // audio can continue play only interrupted by speech
        flags &= ~MEDIA_FLAG_SAVE_PREVIOUS;
    }

#ifndef PSRAM_ENABLED
    if (media_type == TYPE_M4A) {
        DIR* dir = opendir("/sd");
        if (dir == NULL) {
            DUER_LOGW("No sd card, can't play m4a file!");
            return;
        } else {
            closedir(dir);
        }
    }
#endif

    MdmMessage* mdm_msg = (MdmMessage*)MEDIA_MALLOC(sizeof(MdmMessage));
    if (mdm_msg == NULL) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("MdmMessage alloc failed!");
        return;
    }

    int len = strlen(media_url) + 1; //add 1 for terminator
    mdm_msg->media_url_or_path = (char*)MEDIA_MALLOC(len);
    if (mdm_msg->media_url_or_path == NULL) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("Malloc memory for media url to send mdm message failed!");
        FREE(mdm_msg);
        return;
    }

    mdm_msg->position = 0;
    mdm_msg->flags = flags;
    mdm_msg->data_type = MEDIA_TYPE_URL;
    mdm_msg->media_type = media_type;
    snprintf(mdm_msg->media_url_or_path, len, "%s", media_url);

    DUER_LOGD("Sending media url to MDM thread");
    mdm_send_msg_to_mdm_thread(mdm_msg);
}

/**
 * FUNC:
 * void mdm_send_media_file_path(const char* media_file_path)
 *
 * DESC:
 * external  interface to send media file path mdm message to MDM thread Queue
 *
 * PARAM:
 * @param[in] media_file_path: media file path
 */
void mdm_send_media_file_path(const char* media_file_path, int flags) {
    mdm_notify_to_stop();

    if (media_file_path == NULL) {
        DUER_LOGE("media file path is NULL!");
        return;
    }

    MediaType media_type = determine_media_type_by_suffix(media_file_path, false);
    if (media_type == TYPE_NULL) {
        DUER_LOGE("media_file_path: media_type error.");
        return;
    }

    MdmMessage* mdm_msg = (MdmMessage*)MEDIA_MALLOC(sizeof(MdmMessage));
    if (!mdm_msg) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("MdmMessage alloc failed!");
        return;
    }

    int len = strlen(media_file_path) + 1; //add 1 for terminator
    mdm_msg->media_url_or_path = (char*)MEDIA_MALLOC(len);
    if (!mdm_msg->media_url_or_path) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("Malloc memory for media file path to send mdm message failed!");
        FREE(mdm_msg);
        return;
    }

    mdm_msg->position = 0;
    mdm_msg->flags = flags | MEDIA_FLAG_LOCAL;
    mdm_msg->data_type = MEDIA_TYPE_FILE_PATH;
    mdm_msg->media_type = media_type;
    snprintf(mdm_msg->media_url_or_path, len, "%s", media_file_path);
    DUER_LOGD("Sending media file path to MDM thread");
    mdm_send_msg_to_mdm_thread(mdm_msg);
}


void mdm_send_magic_voice(char* data, int size, int flags) {
    mdm_notify_to_stop();

    if (data == NULL) {
        DUER_LOGE("magic_voice data is NULL!");
        return;
    }

    MdmMessage* mdm_msg = (MdmMessage*)MEDIA_MALLOC(sizeof(MdmMessage));
    if (!mdm_msg) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("MdmMessage alloc failed!");
        return;
    }

    mdm_msg->armnb_data = data;
    mdm_msg->data = NULL;
    mdm_msg->data_size = size;
    mdm_msg->media_url_or_path = NULL;
    mdm_msg->position = 0;
    mdm_msg->flags = flags;
    mdm_msg->data_type = MEDIA_TYPE_MAGIC_VOICE;
    mdm_msg->media_type = TYPE_WAV;
    DUER_LOGD("Sending magic_voice to MDM thread");
    mdm_send_msg_to_mdm_thread(mdm_msg);
}


/**
 * FUNC:
 * void mdm_send_media_data(const char* data, size_t size)
 *
 * DESC:
 * external  interface to send media data mdm message to MDM thread Queue
 *
 * PARAM:
 * @param[in] data: media data buffer
 * @param[in] size: media data size
 */
void mdm_send_media_data(const char* data, size_t size, int flags) {
    if (data == NULL || size < 1) {
        DUER_LOGE("Invalid arguments.");
        return;
    }

    mdm_notify_to_stop();

    MdmMessage* mdm_msg = (MdmMessage*)MEDIA_MALLOC(sizeof(MdmMessage));
    if (!mdm_msg) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("MdmMessage alloc failed!");
        return;
    }

    mdm_msg->data = data;
    mdm_msg->data_size = size;
    mdm_msg->media_url_or_path = NULL;
    mdm_msg->position = 0;
    mdm_msg->flags = flags;
    mdm_msg->data_type = MEDIA_TYPE_DATA;
    mdm_msg->media_type = TYPE_MP3;
    DUER_LOGD("Sending media data to MDM thread");
    mdm_send_msg_to_mdm_thread(mdm_msg);
}

/**
 * FUNC:
 * MediaPlayerStatus mdm_notify_to_stop()
 *
 * DESC:
 * external  interface to notify MDM thread to stop getting data
 *
 * PARAM:
 * none
 */
MediaPlayerStatus mdm_notify_to_stop() {
    DUER_LOGD("notify MDM thread to stop providing media data!");

    //s_mdm_stop_mutex.lock();

    s_mdm_stop_flag = 1;
    MediaPlayerStatus status = media_play_stop(s_media_seek_position > -1);
    if (status == MEDIA_PLAYER_PAUSE) {
        s_previous_media_paused = true;
    } else if (status == MEDIA_PLAYER_PLAYING) {
        s_previous_media_paused = false;
    } else {
        // do nothing
    }

    //s_mdm_stop_mutex.unlock();
    return status;
}

/**
 * FUNC:
 * MediaPlayerStatus mdm_notify_to_stop_completely()
 *
 * DESC:
 * external  interface to notify MDM thread to stop getting data completely
 *
 * PARAM:
 * none
 */
MediaPlayerStatus mdm_notify_to_stop_completely() {
    s_stop_completely = true;
    MediaPlayerStatus status = mdm_notify_to_stop();
    if (status == MEDIA_PLAYER_IDLE) {
        reset_previous_data();
    }

    return status;
}

/**
 * FUNC:
 * void mdm_reset_stop_flag()
 *
 * DESC:
 * external  interface to reset MDM stop flag
 *
 * PARAM:
 * none
 */
void mdm_reset_stop_flag() {
    DUER_LOGD("reset mdm stop flag!");

    //s_mdm_stop_mutex.lock();

    s_mdm_stop_flag = 0;
    s_stop_completely = false;

    //s_mdm_stop_mutex.unlock();
}

/**
 * FUNC:
 * int mdm_check_need_to_stop()
 *
 * DESC:
 * check need to stop getting data
 *
 * PARAM:
 * none
 *
 * @RETURN
 * 1: to stop; 0: no stop
 */
int mdm_check_need_to_stop() {
    return s_mdm_stop_flag;
}

/**
 * FUNC:
 * int mdm_play_previous_media_continue()
 *
 * DESC:
 * play the previous media
 *
 * PARAM:
 * none
 *
 * @RETURN
 * -1: failed; 0: success
 */
int mdm_play_previous_media_continue() {
    if (media_play_get_status() == MEDIA_PLAYER_PLAYING) {
        DUER_LOGW("media player is playing now");
        return -1;
    }

    if (s_previous_media.url == NULL) {
        DUER_LOGI("s_previous_media.url is null");
        return MEDIA_ERROR_NO_PREVIOUS_URL;
    }

    if (s_previous_media_paused) {
        DUER_LOGI("you can play previous media continue by press resume button.");
        return 0;
    }

    MdmMessage* msg = (MdmMessage*)MEDIA_MALLOC(sizeof(MdmMessage));
    if (!msg) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("MdmMessage alloc failed!");
        return -1;
    }

    msg->media_url_or_path = s_previous_media.url;
    msg->flags = s_previous_media.flags;
    s_previous_media.url = NULL;
    msg->data_type = s_previous_media.data_type;
    msg->position = s_previous_media.write_size;
    msg->media_type = s_previous_media.media_type;

    mdm_restore_previous_play_progress();

    mdm_send_msg_to_mdm_thread(msg);
    DUER_LOGI("Continue play %s", msg->media_url_or_path);

    return 0;
}

int mdm_set_previous_media(const char *url ,int flags)
{
	int len = strlen(url) + 1;
	
	s_previous_media.url = (char*)MEDIA_MALLOC(len);
	if(!s_previous_media.url){
        DUER_LOGE("previous_media alloc failed!");
        return -1;
    }
		
	memcpy(s_previous_media.url, url, len);
	s_previous_media.flags = flags; 
	s_previous_media.data_type = MEDIA_TYPE_URL;
	s_previous_media.write_size = 0;
	s_previous_media.media_type = TYPE_MP3;

	return 0;
}

/*
 * 0:dcs_audio_url, 1:dcs_speech_url, 2:other_url, 3:local_file, 4:build_in_data
 */
static int mdm_get_detail_source_type(MediaSourceType type, int flags) {
    int source_type = 0;
    switch (type) {
    case MEDIA_TYPE_URL:
        if (flags & MEDIA_FLAG_DCS_URL) {
            source_type = 0;
        } else if (flags & MEDIA_FLAG_SPEECH) {
            source_type = 1;
        } else {
            source_type = 2;
        }
        break;
    case MEDIA_TYPE_FILE_PATH:
        source_type = 3;
        break;
    case MEDIA_TYPE_DATA:
        source_type = 4;
    default:
        // do nothing
        break;
    }

    return source_type;
}

void mdm_reg_play_failed_cb(mdm_media_play_failed_cb cb)
{
    s_play_failed_cb = cb;
}

/**
 * FUNC:
 * void media_data_mgr_thread()
 *
 * DESC:
 * media data managerment thread to receive mdm message
 * to get media data and send to media player thread
 *
 * PARAM:
 * none
 */
static void media_data_mgr_thread() {
    while (true) {
        int ret = 0;
        int source_type = 0;
        bool store_previous_media = false;
        bool clear_previous_media = true;
        MdmMessage* mdm_msg = NULL;

        while (mdm_msg == NULL) {
            s_mdm_msg_sem.wait();
            s_mdm_msg_mutex.lock();
            mdm_msg = s_mdm_msg;
            s_mdm_msg = NULL;
            s_mdm_msg_mutex.unlock();
        }

        DUER_LOGI("MDM thread receive message, data_type :%d, url:%s", mdm_msg->data_type,
                mdm_msg->media_url_or_path);

        source_type = mdm_get_detail_source_type(mdm_msg->data_type, mdm_msg->flags);

        media_play_reset_error();
        s_unsupport_format = false;
        s_media_type = mdm_msg->media_type;

        switch (mdm_msg->data_type) {
        case MEDIA_TYPE_URL:
            PERFORM_STATISTIC_END(PERFORM_STATISTIC_GET_URL,
                                  0,
                                  UNCONTINUOUS_TIME_MEASURE);

			ret = mdm_get_data_by_media_url(mdm_msg->media_url_or_path, mdm_msg->position,
					mdm_msg->flags);
            break;
        case MEDIA_TYPE_FILE_PATH:
            ret = mdm_get_data_by_media_file_path(mdm_msg->media_url_or_path,
                    "audio/mpeg", mdm_msg->position, mdm_msg->flags);
            break;
			
        case MEDIA_TYPE_DATA:
            ret = mdm_send_data_to_buffer(mdm_msg->data, mdm_msg->data_size, mdm_msg->flags);
            break;
		
		case MEDIA_TYPE_MAGIC_VOICE:
			DUER_LOGE("armnb_data[%p],data_size[%d]", mdm_msg->armnb_data, mdm_msg->data_size);
            ret = mdm_send_magic_data_to_buffer(mdm_msg->armnb_data, mdm_msg->data_size, mdm_msg->flags);
            break;
		
        default:
            DUER_LOGE("data_type in message(%d) is error! ", mdm_msg->data_type);
            break;
        }

        if (ret < 0) {
            if (s_play_failed_cb) {
                s_play_failed_cb(mdm_msg->flags);
            }
        }

        /*
         * If media file is very short, media player maybe not start yet.
         * So wait 100 ms here
         */
        Thread::wait(100);
        media_play_wait_end();
        if (!s_stop_completely) {
            // if media type is audio and play was interrupted, save the media info
            if (mdm_msg->flags & MEDIA_FLAG_PROMPT_TONE) {
                clear_previous_media = false;
            } else {
                bool finished = media_play_finished() || media_play_error();
                store_previous_media = (!finished && s_media_type != TYPE_SPEECH);
                // if don't need to save previous media info, clear the saved previous media info
                clear_previous_media = ((mdm_msg->flags & MEDIA_FLAG_SAVE_PREVIOUS) == 0);
            }
        }

        // code for continue play previous media file
        if (store_previous_media) {
            DUER_LOGV("Media play interrupted and media data is not speech\n");
            if (s_previous_media.url != NULL) {
                FREE(s_previous_media.url);
                s_previous_media.url = NULL;
            }

            // store previous media data
            s_previous_media.data_type = mdm_msg->data_type;
            s_previous_media.url = mdm_msg->media_url_or_path;
            s_previous_media.media_type = s_media_type;
            s_previous_media.flags = mdm_msg->flags;

            // not stopped by seek, when s_media_seek_position is -1
            if (s_media_seek_position < 0) {
                s_previous_media.write_size = mdm_msg->position + media_play_get_write_size();
                s_previous_media.flags &= ~MEDIA_FLAG_SEEK;
            } else {
                s_previous_media.write_size = adjust_position(s_media_seek_position);
				YTMediaManager::instance().play_previous_media_continue();
                s_media_seek_position = -1;
                s_previous_media.flags |= MEDIA_FLAG_SEEK;

                DUER_LOGD("Seek %s, position is %d", mdm_msg->media_url_or_path,
                    s_previous_media.write_size);
            }

            mdm_msg->media_url_or_path = NULL;
        } else if (clear_previous_media) {
            DUER_LOGV("clear previous media info!");
            reset_previous_data();
        }

        if (mdm_msg->media_url_or_path != NULL) {
            FREE(mdm_msg->media_url_or_path);
            mdm_msg->media_url_or_path = NULL;
        }
        FREE(mdm_msg);
    }
}

void start_media_data_mgr_thread() {
#ifdef BAIDU_STACK_MONITOR
    register_thread(&s_mdm_thread, "MediaDataManager");
#endif
    s_mdm_thread.start(media_data_mgr_thread);
}

void mdm_get_download_progress(size_t* total_size, size_t* recv_size) {
    if (total_size == NULL || recv_size == NULL) {
        return;
    }

    *total_size = 0;
    *recv_size = 0;

    if (s_http_client) {
        duer_http_get_download_progress(s_http_client, total_size, recv_size);
    }
}

void mdm_seek_media(int position) {
    if (position < 0) {
        return;
    }

    if (s_media_type == TYPE_SPEECH) {
        DUER_LOGW("Media type:%d unsupport seek.", s_media_type);
        return;
    }

    if (s_previous_media_paused) {
        s_previous_media.write_size = adjust_position(s_media_seek_position);
    } else {
        s_media_seek_position = position;
        mdm_notify_to_stop();
    }
}

MediaPlayerStatus mdm_pause_or_resume() {
    MediaPlayerStatus status = media_play_get_status();
    if (status != MEDIA_PLAYER_IDLE) {
        // if hls is playing, stop it. then restart it when resume
		media_play_pause_or_resume();
    } else if (status == MEDIA_PLAYER_IDLE && s_previous_media_paused) {
        // this case means music was interrupted by tts when paused
        // so we should continue play previous music
        s_previous_media_paused = false;
		YTMediaManager::instance().play_previous_media_continue();
        status = MEDIA_PLAYER_PAUSE;
    }

    return status;
}

} // namespace duer
