// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: baidu buffer for c

#include "baidu_buffer.h"
#include <stdlib.h>
#include <string.h>
#include "lightduer_log.h"
#ifdef PSRAM_ENABLED
#include "baidu_psram_partition.h"
#include "baidu_psram_wrapper.h"
#endif

typedef struct DuerCommonBuffer {
    void*  buffer;
    size_t size;
    bool   need_free;
} DuerCommonBuffer;

typedef struct DuerFileBuffer {
    FILE*  file;
    size_t size;
} DuerFileBuffer;

#ifdef PSRAM_ENABLED
typedef struct DuerPsramBuffer {
    uint32_t address;
    size_t   size;
} DuerPsramBuffer;
#endif

static int common_buffer_write(DuerBuffer* duer_buffer, size_t pos,
        const void* buffer, size_t length) {
    if (duer_buffer == NULL || duer_buffer->specific_buffer == NULL || buffer == NULL) {
        DUER_LOGE("Invalid argments.");
        return -1;
    }

    DuerCommonBuffer* common_buffer = (DuerCommonBuffer*)duer_buffer->specific_buffer;
    if (pos + length > common_buffer->size) {
        DUER_LOGE("out of buffer.");
        return -1;
    }

    memcpy((char*)common_buffer->buffer + pos, buffer, length);

    return 0;
}

static int common_buffer_read(DuerBuffer* duer_buffer, size_t pos, void* buffer, size_t length) {
    if (duer_buffer == NULL || duer_buffer->specific_buffer == NULL || buffer == NULL) {
        DUER_LOGE("Invalid argments.");
        return -1;
    }

    DuerCommonBuffer* common_buffer = (DuerCommonBuffer*)duer_buffer->specific_buffer;
    if (pos + length > common_buffer->size) {
        DUER_LOGE("out of buffer.");
        return -1;
    }

    memcpy(buffer, (char*)common_buffer->buffer + pos, length);

    return 0;
}

static size_t common_buffer_size(DuerBuffer* duer_buffer) {
    if (duer_buffer == NULL || duer_buffer->specific_buffer == NULL) {
        DUER_LOGE("Invalid argments.");
        return 0;
    }

    DuerCommonBuffer* common_buffer = (DuerCommonBuffer*)duer_buffer->specific_buffer;

    return common_buffer->size;
}

DuerBuffer* create_common_buffer(void* buffer, size_t size, bool need_free) {
    if (buffer == NULL || size < 1) {
        DUER_LOGE("Invalid argments.");
        return NULL;
    }

    DuerBuffer* duer_buffer = (DuerBuffer*)malloc(sizeof(DuerBuffer));
    if (duer_buffer == NULL) {
        DUER_LOGE("No free memory.");
        return NULL;
    }

    DuerCommonBuffer* common_buffer = (DuerCommonBuffer*)malloc(sizeof(DuerCommonBuffer));
    if (common_buffer == NULL) {
        DUER_LOGE("No free memory.");
        free(duer_buffer);
        return NULL;
    }

    common_buffer->buffer = buffer;
    common_buffer->size = size;
    common_buffer->need_free = need_free;
    duer_buffer->specific_buffer = common_buffer;
    duer_buffer->write = common_buffer_write;
    duer_buffer->read = common_buffer_read;
    duer_buffer->size = common_buffer_size;

    return duer_buffer;
}

void destroy_common_buffer(DuerBuffer* duer_buffer) {
    if (duer_buffer != NULL) {
        if (duer_buffer->specific_buffer != NULL) {
            DuerCommonBuffer* common_buffer = (DuerCommonBuffer*)duer_buffer->specific_buffer;
            if (common_buffer->need_free) {
                free(common_buffer->buffer);
                common_buffer->buffer = NULL;
            }

            free(common_buffer);
            duer_buffer->specific_buffer = NULL;
        }
        free(duer_buffer);
    }
}

static int file_buffer_write(DuerBuffer* duer_buffer, size_t pos,
        const void* buffer, size_t length) {
    if (duer_buffer == NULL || duer_buffer->specific_buffer == NULL || buffer == NULL) {
        DUER_LOGE("Invalid argments.");
        return -1;
    }

    DuerFileBuffer* file_buffer = (DuerFileBuffer*)duer_buffer->specific_buffer;
    if (pos + length > file_buffer->size) {
        DUER_LOGE("out of buffer.");
        return -1;
    }

    fseek(file_buffer->file, pos, SEEK_SET);
    int ret = fwrite(buffer, 1, length, file_buffer->file);
    fflush(file_buffer->file);

    return ret == length ? 0 : -1;
}

static int file_buffer_read(DuerBuffer* duer_buffer, size_t pos, void* buffer, size_t length) {
    if (duer_buffer == NULL || duer_buffer->specific_buffer == NULL || buffer == NULL) {
        DUER_LOGE("Invalid argments.");
        return -1;
    }

    DuerFileBuffer* file_buffer = (DuerFileBuffer*)duer_buffer->specific_buffer;
    if (pos + length > file_buffer->size) {
        DUER_LOGE("out of buffer.");
        return -1;
    }

    fseek(file_buffer->file, pos, SEEK_SET);

    return fread(buffer, 1, length, file_buffer->file) == length ? 0 : -1;;
}

static size_t file_buffer_size(DuerBuffer* duer_buffer) {
    if (duer_buffer == NULL || duer_buffer->specific_buffer == NULL) {
        DUER_LOGE("Invalid argments.");
        return 0;
    }

    DuerFileBuffer* file_buffer = (DuerFileBuffer*)duer_buffer->specific_buffer;

    return file_buffer->size;
}

DuerBuffer* create_file_buffer(FILE* file, size_t size) {
    if (file == NULL || size < 1) {
        DUER_LOGE("Invalid argments.");
        return NULL;
    }

    DuerBuffer* duer_buffer = (DuerBuffer*)malloc(sizeof(DuerBuffer));
    if (duer_buffer == NULL) {
        DUER_LOGE("No free memory.");
        return NULL;
    }

    DuerFileBuffer* file_buffer = (DuerFileBuffer*)malloc(sizeof(DuerFileBuffer));
    if (file_buffer == NULL) {
        DUER_LOGE("No free memory.");
        free(duer_buffer);
        return NULL;
    }

    file_buffer->file = file;
    file_buffer->size = size;
    duer_buffer->specific_buffer = file_buffer;
    duer_buffer->write = file_buffer_write;
    duer_buffer->read = file_buffer_read;
    duer_buffer->size = file_buffer_size;

    return duer_buffer;
}

void destroy_file_buffer(DuerBuffer* duer_buffer) {
    if (duer_buffer != NULL) {
        if (duer_buffer->specific_buffer != NULL) {
            DuerFileBuffer* file_buffer = (DuerFileBuffer*)duer_buffer->specific_buffer;
            fclose(file_buffer->file);
            file_buffer->file = NULL;

            free(duer_buffer->specific_buffer);
            duer_buffer->specific_buffer = NULL;
        }
        free(duer_buffer);
    }
}

#ifdef PSRAM_ENABLED
static int psram_buffer_write(DuerBuffer* duer_buffer, size_t pos,
        const void* buffer, size_t length) {
    if (duer_buffer == NULL || duer_buffer->specific_buffer == NULL || buffer == NULL) {
        DUER_LOGE("Invalid argments.");
        return -1;
    }

    DuerPsramBuffer* psram_buffer = (DuerPsramBuffer*)duer_buffer->specific_buffer;
    if (pos + length > psram_buffer->size) {
        DUER_LOGE("out of buffer.");
        return -1;
    }

    return psram_write_buffer(psram_buffer->address + pos, (void*)buffer, length);
}

static int psram_buffer_read(DuerBuffer* duer_buffer, size_t pos, void* buffer, size_t length) {
    if (duer_buffer == NULL || duer_buffer->specific_buffer == NULL || buffer == NULL) {
        DUER_LOGE("Invalid argments.");
        return -1;
    }

    DuerPsramBuffer* psram_buffer = (DuerPsramBuffer*)duer_buffer->specific_buffer;
    if (pos + length > psram_buffer->size) {
        DUER_LOGE("out of buffer.");
        return -1;
    }

    return psram_read_buffer(psram_buffer->address + pos, buffer, length);
}

static size_t psram_buffer_size(DuerBuffer* duer_buffer) {
    if (duer_buffer == NULL || duer_buffer->specific_buffer == NULL) {
        DUER_LOGE("Invalid argments.");
        return 0;
    }

    DuerPsramBuffer* psram_buffer = (DuerPsramBuffer*)duer_buffer->specific_buffer;

    return psram_buffer->size;
}

DuerBuffer* create_psram_buffer(uint32_t address, size_t size) {
    if (address + size > PSRAM_MAX_SIZE) {
        DUER_LOGE("Invalid argument.");
        return NULL;
    }

    DuerBuffer* duer_buffer = (DuerBuffer*)malloc(sizeof(DuerBuffer));
    if (duer_buffer == NULL) {
        DUER_LOGE("No free memory.");
        return NULL;
    }

    DuerPsramBuffer* psram_buffer = (DuerPsramBuffer*)malloc(sizeof(DuerPsramBuffer));
    if (psram_buffer == NULL) {
        DUER_LOGE("No free memory.");
        free(duer_buffer);
        return NULL;
    }

    psram_buffer->address = address;
    psram_buffer->size = size;
    duer_buffer->specific_buffer = psram_buffer;
    duer_buffer->write = psram_buffer_write;
    duer_buffer->read = psram_buffer_read;
    duer_buffer->size = psram_buffer_size;

    return duer_buffer;
}

void destroy_psram_buffer(DuerBuffer* duer_buffer) {
    if (duer_buffer != NULL) {
        if (duer_buffer->specific_buffer != NULL) {
            free(duer_buffer->specific_buffer);
            duer_buffer->specific_buffer = NULL;
        }
        free(duer_buffer);
    }
}

#endif // PSRAM_ENABLED
