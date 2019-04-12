#ifndef __LAYER_H__
#define __LAYER_H__

#include "Array.h"
#include "mdnn_fileio.h"

typedef enum _Activation_type
{
    Identical=0,
    Sigmoid=1,
    _MAX=0xfffffff  // so that this enum takes 4 bytes
}Activation_type;

class Layer
{
    public:
        Layer();
        ~Layer();
        void configure(MDNN_FILE* fp);
        void forward(Array& out, const Array& in);

        inline void SetId(int new_id) {
            id = new_id;
        }

        inline int GetId() {
            return id;
        }

        inline void SetMaxLayerId(int max_id) {
            max_layer_id = max_id;
        }

        inline float32_type* GetFloatData() {
            return f_temp;
        }

        inline uint64_type GetInputNum() {
            return num_input;
        }

        inline uint64_type GetOutputNum() {
            return num_output;
        }

    private:

        void linear_function(const Array& in);
        void add_bias(float32_type in_scale);
        void activation_function();
        void convert_f2uc(Array& out);
#ifdef __X86__
        void sigmoid();
#endif

        void softmax();


#ifdef __ARM__
        void sigmoid_arm();
#endif

        int id;
        int max_layer_id;

        uint64_type num_input;
        uint64_type num_output;

        Array weight;
        Array bias;
        Activation_type act_type;

        float32_type* f_temp;
        int32_type* i_temp;
};

#endif
