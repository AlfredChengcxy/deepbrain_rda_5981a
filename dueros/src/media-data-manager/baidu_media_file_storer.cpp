// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Media file storer

#include "baidu_media_file_storer.h"
#include "DirHandle.h"
#include "rtos.h"
#include "baidu_media_play.h"
#include "lightduer_log.h"

namespace duer {

static const char CACHE_DIR[] = "/sd/cache";
static const int MAX_FILE_NUM = 100;
MediaFileStorer MediaFileStorer::_s_instance;

MediaFileStorer::MediaFileStorer() : _initialized(false), _file(NULL), _file_index(0) {
}

MediaFileStorer& MediaFileStorer::instance() {
    if (!_s_instance._initialized) {
        _s_instance.initialize();
        _s_instance._initialized = true;
    }
    return _s_instance;
}

void MediaFileStorer::initialize() {
    int res = mkdir(CACHE_DIR, 0777);
    DUER_LOGD("mkdir return %d", res);
}

int MediaFileStorer::open(int type) {
    if (_file != NULL) {
        fclose(_file);
    }
    char* type_str = NULL;
    switch (type) {
    case TYPE_MP3:
    case TYPE_SPEECH:
        type_str = "mp3";
        break;
    case TYPE_AAC:
        type_str = "acc";
        break;
    case TYPE_M4A:
        type_str = "m4a";
        break;
    case TYPE_WAV:
        type_str = "wav";
        break;
    case TYPE_TS:
        type_str = "ts";
        break;
    default:
        DUER_LOGE("Invalid type: %d", type);
        break;
    }

    char path[25];
    snprintf(path, 25, "%s/%d.%s", CACHE_DIR, _file_index, type_str);
    _file = fopen(path, "wb");
    if (_file != NULL) {
        DUER_LOGI("open %s", path);
        _file_index = ++_file_index % MAX_FILE_NUM;
        return 0;
    }
    return -1;
}

int MediaFileStorer::write(const void* buff, size_t size) {
    if (_file != NULL) {
        if (fwrite(buff, size, 1, _file)) {
            fflush(_file);
            return 0;
        } else {
            return -1;
        }
    }
    return -1;
}

int MediaFileStorer::close() {
    if (_file != NULL) {
        fclose(_file);
        _file = NULL;
    }
    return 0;
}

} // namespace duer
