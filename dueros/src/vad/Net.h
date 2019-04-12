#ifndef __NET_H__
#define __NET_H__

#include "Array.h"
#include "Layer.h"
#include "mdnn_fileio.h"

class Net
{
    public:
        Net(char* config_path);
        Net(MDNN_FILE* config_fp);
        void load_data(uint8_type* buf, uint64_type size, float32_type s);
        void work();
        void show_result() const;
        uint64_type get_output_num() const;
        const float32_type* get_output_data() const;

        ~Net();

    private:
        void config_from_fp(MDNN_FILE* fp);

        int32_type num_layers;

        Layer* layer_chain;

        Array input;
        Array output;
};

#endif
