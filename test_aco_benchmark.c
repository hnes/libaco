// Copyright 2018 Sen Han <00hnes@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define _GNU_SOURCE

#include "aco.h"    
#include <alloca.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "aco_assert_override.h"

aco_cofuncp_t gl_co_fp;

#define PRINT_BUF_SZ 64
char gl_benchmark_print_str_buf[64];

void co_fp_alloca(){
    size_t sz = (size_t)((uintptr_t)aco_get_arg());
    uint8_t* ptr = NULL;
    assert(sz > 0);
    ptr = alloca(sz);
    assertptr(ptr);
    memset(ptr, 0, sz);
    while(1){
        aco_yield();
    }
    aco_exit();
}

void co_fp_stksz_128(){
    int ip[28];
    memset(ip, 1, sizeof(ip));
    while(1){
        aco_yield();
    }
    aco_exit();
}

void co_fp_stksz_64(){
    int ip[12];
    memset(ip, 1, sizeof(ip));
    while(1){
        aco_yield();
    }
    aco_exit();
}

void co_fp_stksz_40(){
    int ip[8];
    memset(ip, 1, sizeof(ip));
    while(1){
        aco_yield();
    }
    aco_exit();
}

void co_fp_stksz_24(){
    int ip[4];
    memset(ip, 1, sizeof(ip));
    while(1){
        aco_yield();
    }
    aco_exit();
}

void co_fp_stksz_8(){
    while(1){
        aco_yield();
    }
    aco_exit();
}

void co_fp0(){
    while(1){
        aco_yield();
    }
    aco_exit();
}

void benchmark_copystack(size_t co_amount,size_t stksz, size_t loopct){
    struct timespec tstart={0,0}, tend={0,0};
    int print_sz = 0;
    double delta_t;
    // create co
    assert(co_amount > 0);
    assertptr((void*)gl_co_fp);
    aco_t* main_co = aco_create(NULL, NULL, 0, NULL, NULL);
    aco_share_stack_t* sstk = aco_share_stack_new(0);
    // NOTE: size_t_safe_mul
    aco_t** coarray = (aco_t**) malloc(sizeof(void*) * co_amount);
    assertptr(coarray);
    memset(coarray, 0, sizeof(void*) * co_amount);
    size_t ct = 0;
    assert(0 == clock_gettime(CLOCK_MONOTONIC, &tstart));
    while(ct < co_amount){
        coarray[ct] = aco_create(
            main_co, sstk, 0, gl_co_fp, 
            (void*)((uintptr_t)stksz)
        );
        ct++;
    }
    assert(0 == clock_gettime(CLOCK_MONOTONIC, &tend));
    delta_t = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -
        ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
    //aco_create/init_save_stk_sz=64B    10000000   140.43 ns/op     7126683.67 op/s
    print_sz = snprintf(
        gl_benchmark_print_str_buf, PRINT_BUF_SZ, 
        "aco_create/init_save_stk_sz=64B"
    );
    assert(print_sz > 0 && print_sz < PRINT_BUF_SZ);
    printf("%-50s %11zu %9.3f s %11.2f ns/op %13.2f op/s\n", 
        gl_benchmark_print_str_buf,
        co_amount, delta_t,
        (1.0e+9) / (co_amount / delta_t),
        co_amount / delta_t);
    fflush(stdout);
    // warm-up
    ct = 0;
    while(ct < co_amount){
        aco_resume(coarray[ct]);
        ct++;
    }
    // copystack ctxsw
    assert(0 == clock_gettime(CLOCK_MONOTONIC, &tstart));
    size_t glct = 0;
    while(glct < loopct){
        ct = 0;
        while(ct < co_amount){
            aco_resume(coarray[ct]);
            ct++;
            glct++;
        }
    }
    assert(0 == clock_gettime(CLOCK_MONOTONIC, &tend));    
    delta_t = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -
        ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
    //aco_resume/copy_stack_size=8B      20000000    36.23 ns/op    27614644.57 op/s    
    print_sz = snprintf(
        gl_benchmark_print_str_buf, PRINT_BUF_SZ, 
        "aco_resume/co_amount=%zu/copy_stack_size=%zuB",
        co_amount, coarray[0]->save_stack.max_cpsz
    );
    assert(print_sz > 0 && print_sz < PRINT_BUF_SZ);
    printf("%-50s %11zu %9.3f s %11.2f ns/op %13.2f op/s\n",
        gl_benchmark_print_str_buf, glct, 
        delta_t, (1.0e+9) / (glct / delta_t), 
        glct / delta_t);
    if(co_amount == 1 && coarray[0]->save_stack.max_cpsz == 0){
        printf("%-50s %11zu %9.3f s %11.2f ns/op %13.2f op/s\n",
            "  -> acosw", glct*2, 
            delta_t, (1.0e+9) / (glct*2 / delta_t), 
            glct*2 / delta_t);
    }
    fflush(stdout);
    // co cleaning
    assert(0 == clock_gettime(CLOCK_MONOTONIC, &tstart));
    ct = 0;
    while(ct < co_amount){
        aco_destroy(coarray[ct]);
        coarray[ct] = NULL;
        ct++;
    }
    assert(0 == clock_gettime(CLOCK_MONOTONIC, &tend));
    aco_share_stack_destroy(sstk);
    sstk = NULL;
    aco_destroy(main_co);
    main_co = NULL;
    free(coarray);
    delta_t = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -
        ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
    //aco_destroy                        20000000    21.22 ns/op    47616496.16 op/s
    print_sz = snprintf(
        gl_benchmark_print_str_buf, PRINT_BUF_SZ, 
        "aco_destroy"
    );
    assert(print_sz > 0 && print_sz < PRINT_BUF_SZ);
    printf("%-50s %11zu %9.3f s %11.2f ns/op %13.2f op/s\n\n", 
        gl_benchmark_print_str_buf,
        co_amount, delta_t, 
        (1.0e+9) / (co_amount / delta_t), 
        co_amount / delta_t);
    fflush(stdout);
}

int main() {
#ifdef ACO_USE_VALGRIND
    if(1){
        printf("%s doesn't have valgrind test yet, "
            "so bypass this test right now.\n",__FILE__
        );
        exit(0);
    }
#endif

    aco_thread_init(NULL);

    printf("warm-up:\n");
    gl_co_fp = co_fp_stksz_8;
    benchmark_copystack(200*10000, 10, 20000000);

#ifdef __i386__
    printf("+build:i386\n");
#elif __x86_64__
    printf("+build:x86_64\n");
#endif

#ifdef ACO_CONFIG_SHARE_FPU_MXCSR_ENV
    printf("+build:-DACO_CONFIG_SHARE_FPU_MXCSR_ENV\n");
    printf("+build:share fpu & mxcsr control words between coroutines\n");
#else
    printf("+build:undefined ACO_CONFIG_SHARE_FPU_MXCSR_ENV\n");
    printf("+build:each coroutine maintain each own fpu & mxcsr control words\n");
#endif
#ifdef ACO_USE_VALGRIND
    printf("+build:-DACO_USE_VALGRIND\n");
    printf("+build:valgrind memcheck friendly support enabled\n");
#else
    printf("+build:undefined ACO_USE_VALGRIND\n");
    printf("+build:without valgrind memcheck friendly support\n");
#endif

    printf("\nsizeof(aco_t)=%zu:\n\n", sizeof(aco_t));

    printf("\nstart-test:\n\n");
    printf("%-50s %15s    %15s    %15s   %15s\n\n", 
        "comment", "task_amount", "all_time_cost", "ns_per_op", "speed"
    );

    gl_co_fp = co_fp_stksz_8;
    benchmark_copystack(1, 10, 20000000);

    gl_co_fp = co_fp_stksz_8;
    benchmark_copystack(1, 10, 20000000);

    gl_co_fp = co_fp_stksz_8;
    benchmark_copystack(200*10000, 10, 20000000);
    gl_co_fp = co_fp_stksz_24;
    benchmark_copystack(200*10000, 10, 20000000);
    gl_co_fp = co_fp_stksz_40;
    benchmark_copystack(200*10000, 10, 20000000);
    gl_co_fp = co_fp_stksz_64;
    benchmark_copystack(200*10000, 10, 20000000);
    gl_co_fp = co_fp_stksz_128;
    benchmark_copystack(200*10000, 10, 20000000);

    gl_co_fp = co_fp_alloca;
    benchmark_copystack(200*10000, 150 - 64, 20000000);

    gl_co_fp = co_fp_alloca;
    benchmark_copystack(200*10000, 158 - 64, 20000000);

    gl_co_fp = co_fp_alloca;
    benchmark_copystack(200*10000, 166 - 64, 20000000);

    gl_co_fp = co_fp_alloca;
    benchmark_copystack(200*10000, 256 - 64, 20000000);

    gl_co_fp = co_fp_alloca;
    benchmark_copystack(200*10000, 512 - 64, 20000000);

    gl_co_fp = co_fp_alloca;
    benchmark_copystack(200*10000, 512 - 64, 20000000);

    gl_co_fp = co_fp_alloca;
    benchmark_copystack(100*10000, 1024 - 64, 20000000);

    gl_co_fp = co_fp_alloca;
    benchmark_copystack(100*10000, 1024 - 64, 20000000);

    gl_co_fp = co_fp_alloca;
    benchmark_copystack(10*10000, 1024 - 64, 20000000);

    gl_co_fp = co_fp_alloca;
    benchmark_copystack(10*10000, 2048 - 64, 20000000);

    gl_co_fp = co_fp_alloca;
    benchmark_copystack(10*10000, 4096 - 64, 20000000);

    gl_co_fp = co_fp_alloca;
    benchmark_copystack(10*10000, 8012 - 64, 20000000);

    return 0;
}
