#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __X86__
void chgemv_u_u(int32_t* dst, uint8_t* mat, uint8_t* vec, int h, int w);
void chgemv_u_c(int32_t* dst, uint8_t* mat, int8_t* vec, int h, int w);
void chgemv_c_u(int32_t* dst, int8_t* mat, uint8_t* vec, int h, int w);
void chgemv_c_c(int32_t* dst, int8_t* mat, int8_t* vec, int h, int w);
#endif

#ifdef __ARM__
void neon_v_mul_m_4x8bl_uu(Int_type *dst, D_type *src1, D_type *src2, int *kn);
#endif

#ifdef __cplusplus
}
#endif
