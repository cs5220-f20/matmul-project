/*
  Ideally, you won't need to change this file.  You may want to change
  a few settings to speed debugging runs, but remember to change back
  to the original settings during final testing.

  These hands have touched the file:
    David Bindel
    David Garmire
    Jason Riedy
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <omp.h>

#ifndef COMPILER
#  define COMPILER "unknown"
#endif
#ifndef FLAGS
#  define FLAGS "unknown"
#endif

/*
  Your function _MUST_ have the following signature:
*/
extern const char* dgemm_desc;
extern void square_dgemm();

/*
  We try to run enough iterations to get reasonable timings.  The matrices
  are multiplied at least MIN_RUNS times.  If that doesn't take MIN_SECS
  seconds, then we double the number of iterations and try again.

  You may want to modify these to speed debugging...
*/
#define MIN_RUNS 4
/* #define MIN_SECS 1.0 */
#define MIN_SECS 0.25

/*
  Note the strange sizes...  You'll see some interesting effects
  around some of the powers-of-two.
*/
const int test_sizes[] = {
    31, 32, 96, 97, 127, 128, 129, 191, 192, 229,
#if defined(DEBUG_RUN)
# define MAX_SIZE 229u
#else
    255, 256, 257, 319, 320, 321, 417, 479, 480, 511, 512, 639, 640,
    767, 768, 769, 1023, 1024, 1025, 1525, 1526, 1527
# define MAX_SIZE 1527u
#endif
};

#define N_SIZES (sizeof (test_sizes) / sizeof (int))


/* --
 * Initialize A to random numbers (A is MAX_SIZE * MAX_SIZE)
 */
void matrix_init(double *A)
{
    for (int i = 0; i < MAX_SIZE*MAX_SIZE; ++i) 
        A[i] = drand48();
}


/* --
 * Zero out C (which is MAX_SIZE * MAX_SIZE)
 */
void matrix_clear(double *C)
{
    memset(C, 0, MAX_SIZE * MAX_SIZE * sizeof(double));
}


/* --
 * Check that C = A*B to within roundoff error.
 *
 * We use the fact that dot products satisfy the error bound
 *
 *   float(sum a_i * b_i) = sum a_i * b_i * (1 + delta_i)
 *
 * where delta_i <= n * epsilon.  In order to check your matrix
 * multiply, we compute each element in turn and make sure that
 * your product is within three times the given error bound.
 * We make it three times because there are three sources of
 * error:
 *
 *  - the roundoff error in your multiply
 *  - the roundoff error in our multiply
 *  - the roundoff error in computing the error bound
 *
 *  That last source of error is not so significant, but that's a
 *  story for another day.
 */
void diff_dgemm(const int M, const double *A, const double *B, double *C)
{
    FILE* fp_our  = fopen("dump_our.txt", "w");
    FILE* fp_ref  = fopen("dump_ref.txt", "w");
    FILE* fp_diff = fopen("dump_diff.txt", "w");
    matrix_clear(C);
    square_dgemm(M, A, B, C);
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < M; ++j) {
            double dotprod = 0;
            double errorbound = 0;
            for (int k = 0; k < M; ++k) {
                double prod = A[k*M + i] * B[j*M + k];
                dotprod += prod;
                errorbound += fabs(prod);
            }
            fprintf(fp_our,  " %g", C[j*M+i]);
            fprintf(fp_ref,  " %g", dotprod);
            fprintf(fp_diff, " % 0.0e", C[j*M+i]-dotprod);
        }
        fprintf(fp_our, "\n");
        fprintf(fp_ref, "\n");
        fprintf(fp_diff, "\n");
    }
    fclose(fp_diff);
    fclose(fp_ref);
    fclose(fp_our);
}

/* --
 * Check that C = A*B to within roundoff error.
 *
 * We use the fact that dot products satisfy the error bound
 *
 *   float(sum a_i * b_i) = sum a_i * b_i * (1 + delta_i)
 *
 * where delta_i <= n * epsilon.  In order to check your matrix
 * multiply, we compute each element in turn and make sure that
 * your product is within three times the given error bound.
 * We make it three times because there are three sources of
 * error:
 *
 *  - the roundoff error in your multiply
 *  - the roundoff error in our multiply
 *  - the roundoff error in computing the error bound
 *
 *  That last source of error is not so significant, but that's a
 *  story for another day.
 */
void validate_dgemm(const int M, const double *A, const double *B, double *C)
{
    matrix_clear(C);
    square_dgemm(M, A, B, C);

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < M; ++j) {
            double dotprod = 0;
            double errorbound = 0;
            for (int k = 0; k < M; ++k) {
                double prod = A[k*M + i] * B[j*M + k];
                dotprod += prod;
                errorbound += fabs(prod);
            }
            errorbound *= (M * DBL_EPSILON);
            double err = fabs(C[j*M + i] - dotprod);
            if (err > 3*errorbound) {
                fprintf(stderr, "Matrix multiply failed.\n");
                fprintf(stderr, "C(%d,%d) should be %lg, was %lg\n", i, j,
                        dotprod, C[j*M + i]);
                fprintf(stderr, "Error of %lg, acceptable limit %lg\n",
                        err, 3*errorbound);
                diff_dgemm(M, A, B, C);
                exit(-1);
            }
        }
    }
}


/* --
 * Compute a MFlop/s rate for C += A*B.
 *
 * The code runs the multiplication repeatedly in a loop MIN_RUNS times,
 * then doubles the loop time if it did not take MIN_SECS to perform the
 * run.  This helps us get around the limits of timer resolution.
 */
double time_dgemm(const int M, const double *A, const double *B, double *C)
{
    double secs = -1.0;
    double mflops_sec;
    int num_iterations = MIN_RUNS;
    while (secs < MIN_SECS) {
        matrix_clear(C);
        double start = omp_get_wtime();
        for (int i = 0; i < num_iterations; ++i) {
            square_dgemm(M, A, B, C);
        }
        double finish = omp_get_wtime();
        double mflops = 2.0 * num_iterations * M * M * M / 1.0e6;
        secs = finish-start;
        mflops_sec = mflops / secs;
        num_iterations *= 2;
    }
    return mflops_sec;
}


int main(int argc, char** argv)
{
    if (argc > 2) {
        fprintf(stderr, "Usage: matmul [csv]\n");
        exit(2);
    }
    
    FILE* fp;
    if (argc == 1) {
        const char* exename = argv[0];
        const char* s = exename + strlen(exename);
        for (; s != exename && *s != '-' && *s != '/'; --s);
        char* fname = (char*) malloc(strlen(s) + strlen("timing.csv") + 1);
        strcpy(fname, "timing");
        strcat(fname, s);
        strcat(fname, ".csv");
        fp = fopen(fname, "w");
        free(fname);
    } else 
        fp = fopen(argv[1], "w");
    
    if (!fp) {
        fprintf(stderr, "Could not open '%s' for output\n", argv[1]);
        exit(3);
    }
    
    double* A = (double*) malloc(MAX_SIZE * MAX_SIZE * sizeof(double));
    double* B = (double*) malloc(MAX_SIZE * MAX_SIZE * sizeof(double));
    double* C = (double*) malloc(MAX_SIZE * MAX_SIZE * sizeof(double));

    matrix_init(A);
    matrix_init(B);

    printf("Compiler:\t%s\nOptions:\t%s\nDescription:\t%s\n\n",
           COMPILER, FLAGS, dgemm_desc);

    fprintf(fp, "size,mflop\n");
    for (int i = 0; i < N_SIZES; ++i) {
        const int M = test_sizes[i];
        validate_dgemm(M, A, B, C);
        fprintf(fp, "%u,%lg\n", M, time_dgemm(M, A, B, C));
    }

    free(C);
    free(B);
    free(A);

    fclose(fp);
    return 0;
}

