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

// Test the customization of aco protector.

#include "aco.h"    
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "aco_assert_override.h"

void foo(int ct){
    printf(
        "co:%p save_stack:%p share_stack:%p yield_ct:%d\n",
        aco_get_co(), aco_get_co()->save_stack.ptr,
        aco_get_co()->share_stack->ptr, ct
    );
    aco_yield();
    (*((int*)(aco_get_arg())))++;
}

void co_fp0()
{
    aco_t* this_co = aco_get_co();
    assert(!aco_is_main_co(this_co));
    assert(this_co->fp == (void*)co_fp0);
    assert(this_co->is_end == 0);
    int ct = 0;
    while(ct < 6){
        foo(ct);
        ct++;
    }
    printf(
        "co:%p save_stack:%p share_stack:%p !offending return!\n",
        this_co, this_co->save_stack.ptr,
        this_co->share_stack->ptr
    );
    printf("Intended to Abort to test the aco protector :)\n");
    return;
    aco_exit();
    assert(0);
}

static void co_protector_last_word(){
    aco_t* co = aco_get_co();
    // do some log about the offending `co`
    fprintf(stderr,"error: customized co_protector_last_word triggered \n");
    fprintf(stderr, "error: co:%p should call `aco_exit(co)` instead of direct "
        "`return` in co_fp:%p to finish its execution\n", co, (void*)co->fp);
    assert(0);
}

int main() {
#ifdef ACO_USE_VALGRIND
    if(0){
        printf("%s doesn't have valgrind test yet, "
            "so bypass this test right now.\n",__FILE__
        );
        exit(0);
    }
#endif

    aco_thread_init(co_protector_last_word);

    aco_t* main_co = aco_create(NULL, NULL, 0, NULL, NULL);
    assertptr(main_co);

    aco_share_stack_t* sstk = aco_share_stack_new(0);
    assertptr(sstk);

    int co_ct_arg_point_to_me = 0;
    aco_t* co = aco_create(main_co, sstk, 0, co_fp0, &co_ct_arg_point_to_me);
    assertptr(co);

    int ct = 0;
    while(ct < 6){
        assert(co->is_end == 0);
        aco_resume(co);
        assert(co_ct_arg_point_to_me == ct);

        printf("main_co:%p\n", main_co);
        ct++;
    }
    aco_resume(co);
    assert(co_ct_arg_point_to_me == ct);
    assert(co->is_end);

    printf("main_co:%p\n", main_co);

    printf(
        "\ncopy-stack co:%p:\n    max stack copy size:%zu\n"
        "    save (from share stack to save stack) counter of the private save stack:%zu\n"
        "    restore (from save stack to share stack) counter of the private save stack:%zu\n",
        co, co->save_stack.max_cpsz, 
        co->save_stack.ct_save, 
        co->save_stack.ct_restore
    );
    printf("\n(Since the share stack used by the co has only one user `co`, "
        "so there is no need to save/restore the stack every time during resume &"
        " yield execution, thus you can call it a co has 'standalone stack' "
        "which just is a very special case of copy-stack.)\n");

    aco_destroy(co);
    co = NULL;

    aco_share_stack_destroy(sstk);
    sstk = NULL;

    aco_destroy(main_co);
    main_co = NULL;

    return 0;
}
