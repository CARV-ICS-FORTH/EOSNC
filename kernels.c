
#include "kernels.h"

#define RUN_2(x) x; x;
#define RUN_4(x) RUN_2(x) RUN_2(x)
#define RUN_8(x) RUN_4(x) RUN_4(x)
#define RUN_16(x) RUN_8(x) RUN_8(x)
#define RUN_32(x) RUN_16(x) RUN_16(x)
#define RUN_64(x) RUN_32(x) RUN_32(x)
#define RUN_128(x) RUN_64(x) RUN_64(x)
#define RUN_256(x) RUN_128(x) RUN_128(x)
#define RUN_512(x) RUN_256(x) RUN_256(x)
#define RUN_1024(x) RUN_512(x) RUN_512(x)
#define RUN_2048(x) RUN_1024(x) RUN_1024(x)
#define RUN_4096(x) RUN_2048(x) RUN_2048(x)
#define RUN_8192(x) RUN_4096(x) RUN_4096(x)
#define RUN_16384(x) RUN_8192(x) RUN_8192(x)


#define _kernel_ab(_r,_a,_b) (_r = _a*_b)
#define _kernel_abc(_r,_a,_b,_c) (_r = _a*_b+_c)
#define _kernel_indexed_ab(_r,_i,_a,_b) (_r[_i++] = _a[_i]*_b[_i])
#define _kernel_indexed_abc(_r,_i,_a,_b,_c) (_r[_i++] = _a[_i]*_b[_i]+c)


#define gen_kernels(_N) \
void kernel_ab(dtype r, dtype a, dtype b) \
{ \
    RUN_##_N(_kernel_ab(r,a,b)); \
} \
void kernel_abc(dtype r, dtype a, dtype b, dtype c) \
{\
    RUN_##_N(_kernel_abc(r,a,b,c)); \
}\

//void kernel_indexed_ab(dtype *r, dtype *a, dtype *b) \
//void kernel_indexed_abc(dtype *r, dtype *a, dtype *b dtype *c) \


gen_kernels(1024)

