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

// A naive and pretty simple scheduler demo.

#include "aco.h"    
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "aco_assert_override.h"

void co_fp0(){
    int ct = 0;
    int loop_ct = (int)((uintptr_t)(aco_get_co()->arg));
    if(loop_ct < 0){
        loop_ct = 0;
    }
    while(ct < loop_ct){
        aco_yield();
        ct++;
    }
    aco_exit();
}

int main() {
    aco_thread_init(NULL);

    time_t seed_t = time(NULL);
    assert((time_t)-1 != seed_t);
    srand(seed_t);

    size_t co_amount = 100;

    // create co
    assert(co_amount > 0);
    aco_t* main_co = aco_create(NULL, NULL, 0, NULL, NULL);
    aco_share_stack_t* sstk = aco_share_stack_new(0);
    // NOTE: size_t_safe_mul
    aco_t** coarray = (aco_t**) malloc(sizeof(void*) * co_amount);
    assertptr(coarray);
    memset(coarray, 0, sizeof(void*) * co_amount);
    size_t ct = 0;
    while(ct < co_amount){
#ifdef ACO_USE_VALGRIND
        aco_share_stack_t* private_sstk = aco_share_stack_new2(
            0, ct % 2
        );
        coarray[ct] = aco_create(
            main_co, private_sstk, 0, co_fp0,
            (void*)((uintptr_t)rand() % 1000)
        );
        private_sstk = NULL;
#else
        coarray[ct] = aco_create(
            main_co, sstk, 0, co_fp0,
            (void*)((uintptr_t)rand() % 1000)
        );
#endif
        ct++;
    }

    // naive scheduler with very poor performance (only for demo and testing)
    printf("scheduler start: co_amount:%zu\n", co_amount);
    size_t null_ct = 0;
    while(1){
        ct = 0;
        while(ct < co_amount){
            if(coarray[ct] != NULL){
                aco_resume(coarray[ct]);
                null_ct = 0;
                if(coarray[ct]->is_end != 0){
                    printf("aco_destroy: co:%zu\n", ct);
                    #ifdef ACO_USE_VALGRIND
                        aco_share_stack_t* private_sstk = coarray[ct]->share_stack;
                    #endif
                    aco_destroy(coarray[ct]);
                    coarray[ct] = NULL;
                    #ifdef ACO_USE_VALGRIND
                        aco_share_stack_destroy(private_sstk);
                        private_sstk = NULL;
                    #endif
                }
            } else {
                null_ct++;
                if(null_ct >= co_amount){
                    goto END;
                }
            }
            ct++;
        }
    }
    // co cleaning
    END:
    ct = 0;
    while(ct < co_amount){
        assert(coarray[ct] == NULL);
        ct++;
    }
    aco_share_stack_destroy(sstk);
    sstk = NULL;
    aco_destroy(main_co);
    main_co = NULL;
    free(coarray);

    printf("sheduler exit");
    
    return 0;
}
