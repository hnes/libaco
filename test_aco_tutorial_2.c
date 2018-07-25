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
        "co:%p save_stack:%p share_stack:%p co_exit()\n",
        this_co, this_co->save_stack.ptr,
        this_co->share_stack->ptr
    );
    aco_exit();
    assert(0);
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

    aco_t* main_co = aco_create(NULL, NULL, 0, NULL, NULL);
    assertptr(main_co);

    aco_share_stack_t* sstk = aco_share_stack_new(0);
    assertptr(sstk);
    aco_share_stack_t* sstk2 = aco_share_stack_new(0);
    assertptr(sstk2);

    int co_ct_arg_point_to_me = 0;
    int co2_ct_arg_point_to_me = 0;
    int co3_ct_arg_point_to_me = 0;
    aco_t* co = aco_create(main_co, sstk, 0, co_fp0, &co_ct_arg_point_to_me);
    assertptr(co);
    aco_t* co2 = aco_create(main_co, sstk2, 0, co_fp0, &co2_ct_arg_point_to_me);
    aco_t* co3 = aco_create(main_co, sstk2, 0, co_fp0, &co3_ct_arg_point_to_me);
    assertptr(co2);
    assertptr(co3);

    int ct = 0;
    while(ct < 6){
        assert(co->is_end == 0);
        aco_resume(co);
        assert(co_ct_arg_point_to_me == ct);

        assert(co2->is_end == 0);
        aco_resume(co2);
        assert(co2_ct_arg_point_to_me == ct);

        assert(co3->is_end == 0);
        aco_resume(co3);
        assert(co3_ct_arg_point_to_me == ct);

        printf("main_co:%p\n", main_co);
        ct++;
    }
    aco_resume(co);
    assert(co_ct_arg_point_to_me == ct);
    assert(co->is_end);

    aco_resume(co2);
    assert(co2_ct_arg_point_to_me == ct);
    assert(co2->is_end);

    aco_resume(co3);
    assert(co3_ct_arg_point_to_me == ct);
    assert(co3->is_end);

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

    printf(
        "\ncopy-stack co2:%p:\n    max stack copy size:%zu\n"
        "    save (from share stack to save stack) counter of the private save stack:%zu\n"
        "    restore (from save stack to share stack) counter of the private save stack:%zu\n",
        co2, co2->save_stack.max_cpsz, 
        co2->save_stack.ct_save, 
        co2->save_stack.ct_restore
    );
    printf(
        "\ncopy-stack co3:%p:\n    max stack copy size:%zu\n"
        "    save (from share stack to save stack) counter of the private save stack:%zu\n"
        "    restore (from save stack to share stack) counter of the private save stack:%zu\n",
        co3, co3->save_stack.max_cpsz, 
        co3->save_stack.ct_save, 
        co3->save_stack.ct_restore
    );

    printf("\n(The co2 & co3 share the share stack sstk2, thus it is "
        "necessary to save/restore the stack every time during resume &"
        " yield execution, thus it is a ordinary case of copy-stack.)\n");

    aco_destroy(co);
    co = NULL;
    aco_destroy(co2);
    co2 = NULL;
    aco_destroy(co3);
    co3 = NULL;

    aco_share_stack_destroy(sstk);
    sstk = NULL;
    aco_share_stack_destroy(sstk2);
    sstk2 = NULL;

    aco_destroy(main_co);
    main_co = NULL;

    return 0;
}
