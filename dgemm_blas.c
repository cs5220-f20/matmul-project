#ifdef OSX_ACCELERATE
#  include <Accelerate/Accelerate.h>
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#  include <mkl_cblas.h>
#elif defined(__GNUC__) || defined(__GNUG__)
#  include <cblas.h>
#endif

const char* dgemm_desc = "System CBLAS dgemm.";

void square_dgemm(const int M, double *A, double *B, double *C)
{
    cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans,
                M, M, M,
                1.0, A, M, B, M, 0.0, C, M);
}
