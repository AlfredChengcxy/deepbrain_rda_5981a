#ifndef __HELPER_H__
#define __HELPER_H__

#ifndef __UVISION_VERSION
#include <iostream>
#include <fstream>
#include <sstream>
#endif
#include "types.h"
#include "mdnn_assert.h"
#include "mdnn_log.h"

#if defined __UVISION_VERSION || defined __MDNN_NO_DEBUG__ || defined __MDNN_NO_LOG__
#define __DEBUG_ARRAY_DISABLED__
#endif

template<typename T>
void debug_array(const char* prefix, int id, const char* postfix, T* data, int size) {
#ifndef __DEBUG_ARRAY_DISABLED__
    std::stringstream ss;
    ss << "debug." << prefix << id << postfix;
    std::ofstream fp_out(ss.str(), std::ios::app);
    for (int i = 0; i < size; i++) {
        fp_out << data[i] << " ";
    }
    fp_out << std::endl;
    fp_out.close();
#endif
}

void debug_int8_array(const char* prefix, int id, const char* postfix, int8_type* data, int size);
void debug_int8_array(const char* prefix, int id, const char* postfix, int8_type* data, int height, int width);

#endif
