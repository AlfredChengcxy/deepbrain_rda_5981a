// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Play m4a type file

#include "baidu_media_play_m4a.h"
#include "baidu_media_adapter.h"
#include "heap_monitor.h"
#include "lightduer_log.h"
#ifdef PSRAM_ENABLED
#include "baidu_psram_partition.h"
#include "baidu_psram_buffer.h"
#else
#include "baidu_file_buffer.h"
#endif

// extern void memory_statistics(const char* tag);

namespace duer {

#define MEDIA_MALLOC(size)  MALLOC(size, MEDIA)

#ifndef PSRAM_ENABLED
static const char M4A_FILE_SD[] = "/sd/M4A";
#endif

duer_m4a_file_callback_t MediaPlayM4A::_s_mp4cb;

MediaPlayM4A::MediaPlayM4A(MediaAdapter* media_adapter) :
        _buffer_position(0),
        _m4a_header_size(0),
        _m4a_header_size_saved(0),
        _buff_data_size(0),
        _p_buff_data(NULL),
        _m4a_file_pos(0),
        _m4a_parse_done(false),
        _media_adapter(media_adapter) {
    if (NULL == media_adapter) {
        DUER_LOGE("M4A media_adapter is NULL");
    }
#ifdef PSRAM_ENABLED
    _buffer = NEW(MEDIA) PsramBuffer(PSRAM_M4A_BUFFER_ADDRESS, MAX_M4A_HEADER_SIZE);
#else
    FILE* file = fopen(M4A_FILE_SD, "wb+");
    if (file == NULL) {
        DUER_LOGE("M4A fopen %s fail.", M4A_FILE_SD);
    }
    _buffer = NEW(MEDIA) FileBuffer(file, MAX_M4A_HEADER_SIZE);
#endif
    if (_buffer == NULL) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("No free memory.");
    }
    _s_mp4cb.user_data = this;
    memset(&_m4a_unpacker, 0, sizeof(_m4a_unpacker));
    _s_mp4cb.read = read_callback;
    _s_mp4cb.seek = seek_callback;
    _m4a_unpacker.stream = &_s_mp4cb;
}

MediaPlayM4A::~MediaPlayM4A() {
    if (NULL != _buffer) {
        delete _buffer;
        _buffer = NULL;
    }

    if (NULL != _p_buff_data) {
        FREE(_p_buff_data);
        _p_buff_data = NULL;
    }
    duer_m4a_close_unpacker(&_m4a_unpacker);
}

int MediaPlayM4A::m4a_file_start(const unsigned char* p_buff, int buff_sz) {
    //assume first 128 byte is m4a header ftyp, moov and so on.
    if (NULL == p_buff || buff_sz < 128) {
        DUER_LOGE("M4A m4a_file_start p_buff:%p, buff_sz:%d.", p_buff, buff_sz);
        return M4AERR_ARG;
    }

    _m4a_file_pos += buff_sz;
    DUER_LOGD("ftype:%c%c%c%c", p_buff[4], p_buff[5], p_buff[6], p_buff[7]);
    if ('f' != p_buff[4] || 't' != p_buff[5] || 'y' != p_buff[6] || 'p' != p_buff[7]) {
        DUER_LOGE("M4A ftyp:%c,%c,%c,%c.", p_buff[4], p_buff[5], p_buff[6], p_buff[7]);
        return M4AERR_VAL;
    }
    int box_size = (p_buff[0] << 24) | (p_buff[1] << 16) | (p_buff[2] << 8) | p_buff[3];
    if (1 == box_size) {
        DUER_LOGE("the box size of ftyp is error");
        return M4AERR_VAL;
    }
    DUER_LOGD("M4A ftype size:%d.", box_size);
    _m4a_header_size += box_size;
    if ('m' != p_buff[box_size + 4] || 'o' != p_buff[box_size + 5]
                || 'o' != p_buff[box_size + 6] || 'v' != p_buff[box_size + 7]) {
        DUER_LOGE("M4A moov:%c,%c,%c,%c.", p_buff[4], p_buff[5], p_buff[6], p_buff[7]);
        return M4AERR_VAL;
    }
    _m4a_header_size += (p_buff[box_size] << 24) | (p_buff[box_size + 1] << 16) |
                    (p_buff[box_size + 2] << 8) | p_buff[box_size + 3];
    DUER_LOGD("M4a head size:%d.", _m4a_header_size);

    if (m4a_header_save(p_buff, buff_sz) != M4AERR_OK) {
        return M4AERR_VAL;
    }

    return m4a_header_parse();
}

int MediaPlayM4A::m4a_file_play(const unsigned char* p_buff, int buff_sz) {
    if (NULL == p_buff || 0 == buff_sz) {
        DUER_LOGI("M4A m4a_file_play p_buff:%p, buff_sz:%d.", p_buff, buff_sz);
        return M4AERR_ARG;
    }

    _m4a_file_pos += buff_sz;
    if (_m4a_header_size > 0) {
        if (m4a_header_save(p_buff, buff_sz) != M4AERR_OK) {
            return M4AERR_VAL;
        }

        if (M4AERR_OK != m4a_header_parse()) {
            DUER_LOGE("m4a parse header error.");
            return M4AERR_VAL;
        }
    } else {
        return m4a_frame_to_codec(p_buff, buff_sz);
    }

    return M4AERR_OK;
}

int MediaPlayM4A::m4a_file_end(const unsigned char* p_buff, int buff_sz) {
    int ret = M4AERR_OK;

    if (p_buff != NULL && buff_sz != 0) {
        _m4a_file_pos += buff_sz;
        if (_m4a_header_size > 0) {
            if (m4a_header_save(p_buff, buff_sz) != M4AERR_OK) {
                return M4AERR_VAL;
            }

            if (M4AERR_OK != m4a_header_parse()) {
                DUER_LOGE("m4a parse header error.");
                if (NULL != _p_buff_data) {
                    FREE(_p_buff_data);
                    _p_buff_data = NULL;
                }
                return M4AERR_VAL;
            }
        } else {
            ret = m4a_frame_to_codec(p_buff, buff_sz);
        }
    }

    _media_adapter->write(NULL, 0);
    if (NULL != _p_buff_data) {
        FREE(_p_buff_data);
        _p_buff_data = NULL;
    }

    return ret;
}

int MediaPlayM4A::m4a_header_save(const unsigned char* p_buff, int buff_sz) {
    if (_m4a_header_size <= 0) {
        return M4AERR_OK;
    }
    if (NULL == p_buff || 0 == buff_sz) {
        DUER_LOGI("m4a m4a_header_save:%p, %d.", p_buff, buff_sz);
        return M4AERR_ARG;
    }

    int file_save = (buff_sz < _m4a_header_size) ? buff_sz : _m4a_header_size;
    DUER_LOGD("file_save:%d.", file_save);

    //workaround for fflush
    unsigned char* buffer = NULL;
    const int MIN_LENGTH = 1024;
    if (file_save < MIN_LENGTH) {
        buffer =  (unsigned char*) MEDIA_MALLOC(MIN_LENGTH);
        if (NULL == buffer) {
            //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
            DUER_LOGE("buffer malloc fail.");
            return M4AERR_VAL;
        }
        memset(buffer, 0, MIN_LENGTH);
        memcpy(buffer, p_buff, file_save);

        if (_buffer->write(_buffer_position, buffer, MIN_LENGTH) < 0) {
            DUER_LOGE("M4A m4a_header_save write to buffer failed.");
            FREE(buffer);
            buffer = NULL;
            return M4AERR_VAL;
        }
        _buffer_position += MIN_LENGTH;

        if (NULL != buffer) {
            FREE(buffer);
            buffer = NULL;
        }
    } else {
        if (_buffer->write(_buffer_position, (void*)p_buff, file_save) < 0) {
            DUER_LOGE("M4A m4a_header_save write to buffer failed.");
            return M4AERR_VAL;
        }
        _buffer_position += file_save;
    }

    _m4a_header_size -= file_save;
    _m4a_header_size_saved += file_save;
    int buff_delta = buff_sz - file_save;
    if (buff_delta > 0) {
        _p_buff_data = (unsigned char*)MEDIA_MALLOC(buff_delta);
        if (NULL == _p_buff_data) {
            //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
            DUER_LOGE("m4a m4a_header_save  malloc fail.");
            return M4AERR_MAC;
        }
        memcpy(_p_buff_data, (p_buff + file_save), (buff_delta));
        _buff_data_size = buff_sz - file_save;
    }

    DUER_LOGD("_m4a_header_size:%d.", _m4a_header_size);
    return M4AERR_OK;
}

int MediaPlayM4A::m4a_header_parse() {
    int ret = M4AERR_OK;

    if (_m4a_header_size <= 0 && !_m4a_parse_done) {
        do {
            ret = duer_m4a_open_unpacker(&_m4a_unpacker);
            if (ret < 0) {
                break;
            }

            _track = duer_m4a_get_aac_track(&_m4a_unpacker);
            if (_track < 0) {
                DUER_LOGE("M4A track:%d error.", _track);
                ret = M4AERR_VAL;
                break;
            }

            unsigned char* buffer = NULL;
            uint32_t buffer_size = 0;

            ret = duer_m4a_get_decoder_config(&_m4a_unpacker, _track, &buffer, &buffer_size);
            if (ret < 0) {
                break;
            }

            if (buffer) {
                ret = duer_m4a_get_audio_specific_config(buffer, buffer_size, &_mp4_asc, 0);
                FREE(buffer);
                buffer = NULL;
                if (ret < 0) {
                    break;
                }
            }
            DUER_LOGD("M4A objectTypeIndex=%d,samplingFrequencyIndex=%d,channelsConfiguration=%d",
                    _mp4_asc.objectTypeIndex, _mp4_asc.samplingFrequencyIndex,
                    _mp4_asc.channelsConfiguration);
            _num_samples = duer_m4a_get_samples_number(&_m4a_unpacker, _track);
            DUER_LOGD("M4A numSamples:%d", _num_samples);
            _sample_id = 0;
            ret = duer_m4a_fill_stsz_table(&_m4a_unpacker, _track, _sample_id);
            DUER_LOGD("M4A duer_m4a_fill_stsz_table:%d", result);
            if (ret < 0) {
                DUER_LOGE("M4A Fill stsz table failed");
                break;
            }
            _m4a_parse_done = true;

            _media_adapter->start_play(TYPE_M4A, 0);
        } while (0);

        if (ret == M4A_ERROR_NO_MEMORY) {
            //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
            ret = M4AERR_MAC;
        } else if (ret < 0) {
            ret = M4AERR_VAL;
        }
    }

    return ret;
}

int MediaPlayM4A::m4a_frame_to_codec(const unsigned char* p_buff, int buff_sz) {
    DUER_LOGV("enter m4a_frame_to_codec _m4a_parse_done:%d.", _m4a_parse_done);
    //memory_statistics("start m4a_frame_to_codec");

    if (!_m4a_parse_done) {
        return M4AERR_OK;
    }

    int32_t buff_data_size = _buff_data_size + buff_sz;
    if (buff_data_size <= 0) {
        DUER_LOGD("m4a_frame_to_codec buff_data_size:%d.", buff_data_size);
        return M4AERR_OK;
    }
    DUER_LOGV("buff_data_size:%d.", buff_data_size);

    unsigned char* buff_data = (unsigned char*)MEDIA_MALLOC(buff_data_size);
    if (NULL == buff_data) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("M4A frame_to_codec buff_data malloc fail.");
        return M4AERR_MAC;
    }

    int temp = 0;
    if (NULL != _p_buff_data && 0 != _buff_data_size) {
        memcpy(buff_data, _p_buff_data, _buff_data_size);
        FREE(_p_buff_data);
        _p_buff_data = NULL;
        temp = _buff_data_size;
        _buff_data_size = 0;
    }
    if (NULL != p_buff && 0 != buff_sz) {
        memcpy(buff_data + temp, p_buff, buff_sz);
    }

    int32_t offset = 0;
    int32_t size = 0;
    int ret = M4AERR_OK;
    while (_sample_id < _num_samples) {
        //int result = mp4ff_get_sample_duration(&_m4a_unpacker, _track, _sample_id);
        //DUER_LOGD("SamDru:%d, sample_id:%d.", result, _sample_id);
        DUER_LOGV("sample_id:%d.", _sample_id);
        duer_m4a_get_sample_position_and_size(&_m4a_unpacker, _track, _sample_id, &offset, &size);
        DUER_LOGV("frame_to_codec:(_m4_file_pos:%d) (offset,%d) (size:%d).",
            _m4a_file_pos, offset, size);

        if (offset + size <= _m4a_file_pos) {
            //ADTS head size is 7 Bytes
            int adts_size = size + ADTS_HEAD_SIZE;
            unsigned char* buff_temp = (unsigned char*)MEDIA_MALLOC(adts_size);
            if (NULL == buff_temp) {
                //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
                DUER_LOGE("M4A frame_to_codec buff_temp malloc fail.");
                ret = M4AERR_MAC;
                break;
            }
            duer_m4a_make_adts_header(buff_temp, adts_size, &_mp4_asc);
            unsigned char* buff_data_offset = buff_data + buff_data_size - _m4a_file_pos + offset;
            memcpy(buff_temp + ADTS_HEAD_SIZE, buff_data_offset, size);

            //send data to codec
            unsigned char* bufptr = buff_temp;
            int bytes = adts_size;
            int num = 0;
            while (bytes > 0) {
                num = (bytes < 2048) ? bytes : 2048;
                _media_adapter->write(bufptr, num);
                bytes -= num;
                bufptr += num;
            }

            FREE(buff_temp);

            _sample_id++;
            if (duer_m4a_refill_stsz_table(&_m4a_unpacker, _track, _sample_id) < 0) {
                ret = M4AERR_VAL;
                break;
            }
        } else if (offset < _m4a_file_pos) {
            DUER_LOGV("_m4a_file_pos:%d, offset:%d", _m4a_file_pos, offset);
            if ((_buff_data_size = (_m4a_file_pos - offset)) > 0) {
                if (NULL == (_p_buff_data = (unsigned char*)MEDIA_MALLOC(_buff_data_size))) {
                    //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
                    DUER_LOGE("M4A frame_to_codec _p_buff_data malloc fail.");
                    ret = M4AERR_MAC;
                    break;
                }
                memcpy(_p_buff_data, buff_data + buff_data_size - _m4a_file_pos + offset,
                        _buff_data_size);
            }
            break;
        } else {
            DUER_LOGV("drop data");
            break;
        }
    }

    FREE(buff_data);
    //memory_statistics("end m4a_frame_to_codec");
    return ret;
}

uint32_t MediaPlayM4A::read_callback(void* user_data, void* buffer, uint32_t length) {
    MediaPlayM4A* p_this = (MediaPlayM4A*)user_data;
    if (p_this == NULL || p_this->_buffer == NULL) {
        return 0;
    }

    if (p_this->_buffer->read(p_this->_buffer_position, buffer, length) < 0) {
        return 0;
    }
    p_this->_buffer_position += length;

    return length;
}

uint32_t MediaPlayM4A::seek_callback(void* user_data, uint64_t position) {
    MediaPlayM4A* p_this = (MediaPlayM4A*)user_data;
    if (p_this != NULL) {
        p_this->_buffer_position = position;
    }

    return 0;
}

int MediaPlayM4A::m4a_file_seek(int file_pos) {
    if (file_pos <= 0) {
        return 0;
    }

    if (file_pos == _m4a_file_pos) {
        return file_pos;
    }

    if (file_pos <= _m4a_header_size_saved || _m4a_header_size > 0) {
        _m4a_file_pos = _m4a_header_size_saved;
        _sample_id = 0;
        DUER_LOGD("m4a_file_seek header has not been parsed.");
        return _m4a_file_pos;
    }

    int32_t size = 0;
    int32_t pos = 0;

    while (_sample_id >= 0 && _sample_id < _num_samples) {
        duer_m4a_get_sample_position_and_size(&_m4a_unpacker, _track, _sample_id, &pos, &size);
        if (file_pos > _m4a_file_pos) {
            if (pos >= file_pos) {
                break;
            }
            _sample_id++;
        } else {
            if (pos < file_pos) {
                break;
            }
            _sample_id--;
        }
        duer_m4a_refill_stsz_table(&_m4a_unpacker, _track, _sample_id);
    }

    _m4a_file_pos = pos;
    DUER_LOGD("m4a_file_seek _m4a_file_pos = %d, _sample_id = %d", _m4a_file_pos, _sample_id);

    return _m4a_file_pos;
}

} // namespace duer
