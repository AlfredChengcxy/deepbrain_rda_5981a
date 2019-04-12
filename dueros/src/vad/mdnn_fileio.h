#ifndef __MDNN_FILEIO_H__
#define __MDNN_FILEIO_H__

#include <stdio.h>
typedef FILE MDNN_FILE;

void mdnn_fread(void* target, size_t elem_size, size_t elem_count, MDNN_FILE* fp);
MDNN_FILE* mdnn_fopen(const char* path, const char* mode);
void mdnn_fclose(MDNN_FILE* fp);

#endif
