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

// A pretty simple scheduler demo using aco_yield_to().

#include "aco.h"    
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "aco_assert_override.h"

size_t curr_co_amount;
size_t curr_co_index;
aco_t** coarray;

void yield_to_next_co(){
    assert(curr_co_amount > 0);
    curr_co_index = (curr_co_index + 1) % curr_co_amount;
    aco_yield_to(coarray[curr_co_index]);
}

void co_fp0(){
    int ct = 0;
    int loop_ct = (int)((uintptr_t)(aco_get_co()->arg));
    if(loop_ct < 0){
        loop_ct = 0;
    }
    while(ct < loop_ct){
        yield_to_next_co();
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
    curr_co_amount = co_amount;

    // create co
    assert(co_amount > 0);
    aco_t* main_co = aco_create(NULL, NULL, 0, NULL, NULL);
    // NOTE: size_t_safe_mul
    coarray = (aco_t**) malloc(sizeof(void*) * co_amount);
    assertptr(coarray);
    memset(coarray, 0, sizeof(void*) * co_amount);
    size_t ct = 0;
    while(ct < co_amount){
        aco_share_stack_t* private_sstk = aco_share_stack_new2(
            0, ct % 2
        );
        coarray[ct] = aco_create(
            main_co, private_sstk, 0, co_fp0,
            (void*)((uintptr_t)rand() % 1000)
        );
        private_sstk = NULL;
        ct++;
    }

    // naive scheduler
    printf("scheduler start: co_amount:%zu\n", co_amount);
    aco_t* curr_co = coarray[curr_co_index];
    while(curr_co_amount > 0){
        aco_resume(curr_co);
        // Update curr_co because aco_yield_to() may have changed it
        curr_co = coarray[curr_co_index];
        assert(curr_co->is_end != 0);
        printf("aco_destroy: co currently at:%zu\n", curr_co_index);
        aco_share_stack_t* private_sstk = curr_co->share_stack;
        aco_destroy(curr_co);
        aco_share_stack_destroy(private_sstk);
        private_sstk = NULL;
        curr_co_amount--;
        if(curr_co_index < curr_co_amount){
            coarray[curr_co_index] = coarray[curr_co_amount];
        }else{
            curr_co_index = 0;
        }
        coarray[curr_co_amount] = NULL;
        curr_co = coarray[curr_co_index];
    }

    // co cleaning
    ct = 0;
    while(ct < co_amount){
        assert(coarray[ct] == NULL);
        ct++;
    }
    aco_destroy(main_co);
    main_co = NULL;
    free(coarray);

    printf("sheduler exit");
    
    return 0;
}
