#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <time.h>
#include <omp.h>
#include <errno.h>
#include "kernels.h"

#define CLOCK_TO_USE CLOCK_MONOTONIC_RAW
//#define CLOCK_TO_USE CLOCK_MONOTONIC_COARSE


#define timespec_to_ns(ts) ((ts)->tv_nsec + (uint64_t)(ts)->tv_sec * 1000000000)
#define MAX_FNAME 64
#define FNAME_PREFIX "samples_from_thread_"
#define FNAME_POSTFIX ".csv"

uint64_t number_of_omp_threads;
uint64_t *kernel_execution_samples;
uint64_t *kernel_timestamp_samples;
uint64_t *instrumentation_overhead_samples;

uint64_t get_time_in_ns(struct timespec *tspec)
{
    clock_gettime(CLOCK_TO_USE, tspec);
    return timespec_to_ns(tspec);
}


uint64_t initialize(uint64_t number_of_samples)
{
    uint64_t i, j, number_of_samples_per_thread;
    number_of_omp_threads = omp_get_max_threads();
    number_of_samples_per_thread = number_of_samples / number_of_omp_threads; 
    kernel_execution_samples = (uint64_t *)malloc(sizeof(uint64_t) * number_of_samples);
    kernel_timestamp_samples = (uint64_t *)malloc(sizeof(uint64_t) * number_of_samples);  // NEW: Allocate timestamp array
    instrumentation_overhead_samples = (uint64_t *)malloc( sizeof(uint64_t) * number_of_samples);
    if(!instrumentation_overhead_samples || !kernel_execution_samples || !kernel_timestamp_samples)
    {
        printf("failed to allocate required memory to hold all the samples");
        exit(-ENOMEM);
    }
#ifndef DONT_TOUCH_BUFFERS_BEFORE_INIT
    for(i=0; i<number_of_samples ; ++i)
    {
        instrumentation_overhead_samples[i] = 0;
        kernel_execution_samples[i] = 0;
        kernel_timestamp_samples[i] = 0;
    }
#endif
    return number_of_samples_per_thread;
}


int main(int argc, char *argv[])
{
    uint64_t sample_num = 0, per_thread_sample_num = 0, i;
    uint64_t kernel_max=0, kernel_min=0, instrumentation_min=0 , instrumentation_max=0;
    struct timespec clock_resolution, instrumentation_ts;
    char *instrumentation_samples_fname = "samples_instrumentation_samples.csv";
    double mean_kernel = 0.0, variance_kernel = 0.0, 
           mean_instrumentation = 0.0, variance_instrumentation = 0.0;
    FILE *inst_fp;
    dtype a,b,c,r;
    
    if(argc != 2)
    {
        printf("usage: %s <number of samples>\n", argv[0]);
        return -EINVAL;
    }
    errno = 0;
    sample_num = strtol(argv[1], NULL, 0);
    if(errno || !sample_num)
    {
        printf("Invalid number of samples\n");
        return -EINVAL;
    }
    per_thread_sample_num = initialize(sample_num);
    printf("OS noise characterisation\n");
    printf("number of threads: %lu\n", number_of_omp_threads);
    printf("kernel size: %d\n", KERN_SIZE);
    printf("numer of samples: %lu\n", sample_num);
    printf("number of samples per thread: %lu\n", per_thread_sample_num);
    clock_getres(CLOCK_TO_USE, &clock_resolution);
    printf("clock resolution: %lu\n", timespec_to_ns(&clock_resolution));
    for(i = 0; i < sample_num; ++i)
    {
        instrumentation_overhead_samples[i] = get_time_in_ns(&instrumentation_ts);
    }

    #pragma omp parallel num_threads(number_of_omp_threads)
    {
        uint64_t j, start, stop;
        uint64_t *sample_slice;
        uint64_t *timestamp_slice;
        char result_fname[MAX_FNAME];
        dtype r, a, b;
        struct timespec slice_ts;
        int id = omp_get_thread_num();
        sample_slice = kernel_execution_samples + id * per_thread_sample_num;
        timestamp_slice = kernel_timestamp_samples + id * per_thread_sample_num;
    #pragma omp barrier
        for(j = 0; j < per_thread_sample_num; ++j)
        {
            r = 0.0;
            a = 1.0002;
            b = 1.9999;
            start = get_time_in_ns(&slice_ts);
#ifdef OUTER_LOOP_THREAD_SYNC
    #pragma omp barrier
#endif
            kernel_ab(r, a, b);
            stop = get_time_in_ns(&slice_ts);
            sample_slice[j] = stop - start;
            timestamp_slice[j] = start;
        }
    #pragma omp barrier
        sprintf(result_fname, "%s%d%s", FNAME_PREFIX, id, FNAME_POSTFIX);
#ifndef SKIP_RAW_DATA
        FILE *fp = fopen(result_fname, "w");
        fprintf(fp, "timestamp_ns,duration_ns\n");
        for(j = 0; j < per_thread_sample_num; ++j)
        {
            fprintf(fp, "%lu,%lu\n", timestamp_slice[j], sample_slice[j]);
        }
        fclose(fp);
#endif
    }
#ifndef SKIP_RAW_DATA
    inst_fp = fopen(instrumentation_samples_fname, "w");
#endif

    for(i = 0; i < sample_num-1; ++i)
    {
        mean_kernel += ((double)kernel_execution_samples[i]) / ((double) sample_num);
        instrumentation_overhead_samples[i] = instrumentation_overhead_samples[i+1] - instrumentation_overhead_samples[i];
        mean_instrumentation += ((double)instrumentation_overhead_samples[i]) / ((double)sample_num);
#ifndef SKIP_RAW_DATA
        fprintf(inst_fp, "%lu\n", instrumentation_overhead_samples[i]);
#endif
    }
    mean_kernel += ((double)kernel_execution_samples[sample_num-1]) / ((double) sample_num);
    instrumentation_overhead_samples[sample_num-1] = instrumentation_overhead_samples[0];
    mean_instrumentation += ((double)instrumentation_overhead_samples[sample_num-1]) / ((double)sample_num);
#ifndef SKIP_RAW_DATA
    fprintf(inst_fp, "%lu\n", instrumentation_overhead_samples[sample_num-1]);
#endif
    kernel_max = kernel_execution_samples[0];
    kernel_min = kernel_execution_samples[0];
    instrumentation_max = instrumentation_overhead_samples[0];
    instrumentation_min = instrumentation_overhead_samples[0];
    for(i = 0; i < sample_num; ++i)
    {
        variance_kernel += (((double)kernel_execution_samples[i] - mean_kernel) *
                            ((double)kernel_execution_samples[i] - mean_kernel)) 
                             /(double)sample_num;
        variance_instrumentation += (((double)instrumentation_overhead_samples[i] - mean_instrumentation) *
                                     ((double)instrumentation_overhead_samples[i] - mean_instrumentation)) 
                                      /(double)sample_num;
        kernel_max = kernel_execution_samples[i] > kernel_max ? kernel_execution_samples[i] : kernel_max ;
        kernel_min = kernel_execution_samples[i] < kernel_min ? kernel_execution_samples[i] : kernel_min ;
        instrumentation_max = instrumentation_overhead_samples[i] > instrumentation_max ? instrumentation_overhead_samples[i] : instrumentation_max;
        instrumentation_min = instrumentation_overhead_samples[i] < instrumentation_min ? instrumentation_overhead_samples[i] : instrumentation_min;
    }
#ifndef SKIP_RAW_DATA
    fclose(inst_fp);
#endif
    printf("\n\n ==== RESULTS ====\n\n");
    printf("noise mean:                        %lf\n", mean_kernel);
    printf("noise variance:                    %lf\n", variance_kernel);
    printf("noise max:                         %lu\n", kernel_max);
    printf("noise min:                         %lu\n", kernel_min);
    printf("instrumentation overhead mean:     %lf\n", mean_instrumentation);
    printf("instrumentation overhead variance: %lf\n", variance_instrumentation);
    printf("instrumentation max:               %lu\n", instrumentation_max);
    printf("instrumentation min:               %lu\n", instrumentation_min);
    return 0;
}

