#ifndef __ARRAY_H__
#define __ARRAY_H__

#include "types.h"
#include "mdnn_fileio.h"
//#include <stdio.h>

const int strideH = 8;
const int strideW = 8;

class Array
{
    public:
        Array();
        Array(uint64_type h, uint64_type w, uint64_type lh, uint64_type lw);
        void load(uint64_type h, uint64_type w, float32_type s, uint8_type* _data);
        Array& operator= (const Array& rhs);
        Array(const Array& rhs);
        ~Array();
        void copy_setnull(Array& rhs);
        void load_from_file(MDNN_FILE* fp);
        void zero();
#ifdef __ARM__
        void resize();
        void div4x8();
        void print2() const;
#endif
        uint8_type* row(int i) const;
        uint64_type getH() const {return height;}
        uint64_type getW() const {return width;}
        int getLH() const {return leadingH;}
        int getLW() const {return leadingW;}
        uint8_type* get_data() const {return data;}
        float32_type get_scale() const {return scale;}
        void set_scale(float32_type s) {scale=s;}

        void output(char* file_path) const;
        void print() const;
        void transpose();

    private:
        uint64_type height;
        uint64_type width;
        int leadingH;
        int leadingW;
        float32_type scale;
        uint8_type* data;
};
#endif
