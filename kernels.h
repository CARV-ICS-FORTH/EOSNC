#ifndef DTYPE
typedef float dtype;
#else
typedef DTYPE dtype;
#endif

#ifndef KERN_SIZE
#define KERN_SIZE 512
#endif

void kernel_ab(dtype r, dtype a, dtype b);
void kernel_abc(dtype r, dtype a, dtype b, dtype c);
//void kernel_indexed_ab(dtype *r, dtype *a, dtype *b);
//void kernel_indexed_abc(dtype *r, dtype *a, dtype *b dtype *c);

