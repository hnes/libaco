# Name

libaco - A blazing fast and lightweight pure C asymmetric coroutine library.

The code name of this project is Arkenstone 💎

Asymmetric COroutine & Arkenstone is the reason why it's been named `aco`.

Support Sys V ABI of Intel386 and x86-64 currently.

Issues and PRs are welcome 🎉🎉🎉

# Table of Content

   * [Name](#name)
   * [Table of Content](#table-of-content)
   * [Status](#status)
   * [Synopsis](#synopsis)
   * [Description](#description)
   * [Build and Test](#build-and-test)
      * [CFLAGS](#cflags)
      * [Build](#build)
      * [Test](#test)
   * [Tutorials](#tutorials)
   * [API](#api)
      * [aco_thread_init](#aco_thread_init)
      * [aco_share_stack_new](#aco_share_stack_new)
      * [aco_share_stack_new2](#aco_share_stack_new2)
      * [aco_share_stack_destroy](#aco_share_stack_destroy)
      * [aco_create](#aco_create)
      * [aco_resume](#aco_resume)
      * [aco_yield](#aco_yield)
      * [aco_get_co](#aco_get_co)
      * [aco_get_arg](#aco_get_arg)
      * [aco_exit](#aco_exit)
      * [aco_destroy](#aco_destroy)
   * [Benchmark](#benchmark)
   * [Proof of Correctness](#proof-of-correctness)
      * [Running Model](#running-model)
      * [Mathematical Induction](#mathematical-induction)
      * [Miscellaneous](#miscellaneous)
         * [Red Zone](#red-zone)
         * [Stack Pointer](#stack-pointer)
   * [Best Practice](#best-practice)
   * [Donation](#donation)
* [Copyright and License](#copyright-and-license)

# Status

Production ready.

# Synopsis

```c
#include "aco.h"    
#include <stdio.h>

void foo(int ct) {
    printf("co: %p: yield to main_co: %d\n", aco_get_co(), *((int*)(aco_get_arg())));
    aco_yield();
    *((int*)(aco_get_arg())) = ct + 1;
}

void co_fp0() {
    printf("co: %p: entry: %d\n", aco_get_co(), *((int*)(aco_get_arg())));
    int ct = 0;
    while(ct < 6){
        foo(ct);
        ct++;
    }
    printf("co: %p:  exit to main_co: %d\n", aco_get_co(), *((int*)(aco_get_arg())));
    aco_exit();
}

int main() {
    aco_thread_init(NULL);

    aco_t* main_co = aco_create(NULL, NULL, 0, NULL, NULL);
    aco_share_stack_t* sstk = aco_share_stack_new(0);

    int co_ct_arg_point_to_me = 0;
    aco_t* co = aco_create(main_co, sstk, 0, co_fp0, &co_ct_arg_point_to_me);

    int ct = 0;
    while(ct < 6){
        assert(co->is_end == 0);
        printf("main_co: yield to co: %p: %d\n", co, ct);
        aco_resume(co);
        assert(co_ct_arg_point_to_me == ct);
        ct++;
    }
    printf("main_co: yield to co: %p: %d\n", co, ct);
    aco_resume(co);
    assert(co_ct_arg_point_to_me == ct);
    assert(co->is_end);

    printf("main_co: destroy and exit\n");
    aco_destroy(co);
    co = NULL;
    aco_share_stack_destroy(sstk);
    sstk = NULL;
    aco_destroy(main_co);
    main_co = NULL;

    return 0;
}
```
```bash
# require gcc version >= 5.0
# default build
$ gcc -g -O2 acosw.S aco.c test_aco_synopsis.c -o test_aco_synopsis
$ ./test_aco_synopsis
main_co: yield to co: 0x1887120: 0
co: 0x1887120: entry: 0
co: 0x1887120: yield to main_co: 0
main_co: yield to co: 0x1887120: 1
co: 0x1887120: yield to main_co: 1
main_co: yield to co: 0x1887120: 2
co: 0x1887120: yield to main_co: 2
main_co: yield to co: 0x1887120: 3
co: 0x1887120: yield to main_co: 3
main_co: yield to co: 0x1887120: 4
co: 0x1887120: yield to main_co: 4
main_co: yield to co: 0x1887120: 5
co: 0x1887120: yield to main_co: 5
main_co: yield to co: 0x1887120: 6
co: 0x1887120:  exit to main_co: 6
main_co: destroy and exit
# i386
$ gcc -g -m32 -O2 acosw.S aco.c test_aco_synopsis.c -o test_aco_synopsis
# share fpu and mxcsr env
$ gcc -g -D ACO_CONFIG_SHARE_FPU_MXCSR_ENV -O2 acosw.S aco.c test_aco_synopsis.c -o test_aco_synopsis 
# with valgrind friendly support
$ gcc -g -D ACO_USE_VALGRIND -O2 acosw.S aco.c test_aco_synopsis.c -o test_aco_synopsis
$ valgrind --leak-check=full --tool=memcheck ./test_aco_synopsis
```

For more information you may refer the "[Build and Test](#build-and-test)" part.

# Description

![thread_model_0](img/thread_model_0.png)

There is 4 basic elements for a ordinary execution state: `{cpu_registers, code, heap, stack}`. 

Since the code information is indicated by `({E|R})?IP` register, and the address of the memory allocated from heap is normally stored in the stack directly or indirectly, thus we could simplify the 4 elements into only 2 of them: `{cpu_registers, stack}`.

![thread_model_1](img/thread_model_1.png)

We define the `main co` as the coroutine who monopolizes the default stack of the current thread. And since the main co is the only user of this stack, we only need to save/restore the necessary cpu registers' state of the main co when it's been yielded-from/resumed-to (switched-out/switched-in).

Next, the definition of the `non-main co` is the coroutine whose execution stack is a stack which is not the default stack of the current thread and may be shared with the other non-main co. Thus the non-main co must have a `private save stack` memory buffer to save/restore its execution stack when it is been switched-out/switched-in (because the succeeding/preceding co may would/had use/used the share stack as its execution stack).

![thread_model_2](img/thread_model_2.png)

There is a special case of non-main co, that is `standalone non-main co` what we called in libaco: the share stack of the non-main coroutine has only one co user. Thus there is no need to do saving/restoring stuff of its private save stack when it is been switched-out/switched-in since there is no other co will touch the execution stack of the standalone non-main co except itself.

![thread_model_3](img/thread_model_3.png)

Finally, we get the big picture of libaco.

There is a "[Proof of Correctness](#proof-of-correctness)" part you may find really helpful if you want to dive into the internal of libaco or want to implement your own coroutine library.

It is also highly recommended to read the source code of the tutorials and benchmark next. The benchmark result is very impressive and enlightening too.

# Build and Test

## CFLAGS

* `-m32`

The `-m32` option of gcc could help you to build the i386 application of libaco on a x86_64 machine. 

* C macro: `ACO_CONFIG_SHARE_FPU_MXCSR_ENV`

You could define the global C macro `ACO_CONFIG_SHARE_FPU_MXCSR_ENV` to speed up the performance of context switching between coroutines slightly if none of your code would change the control words of FPU and MXCSR. If the macro is not defined, all the co would maintain its own copy of the FPU and MXCSR control words. It is recommended to always define this macro globally since it is very rare that one function needs to set its own special env of FPU or MXCSR instead of using the default env defined by the ISO C. But you may not need to define this macro if you are not sure of it.

* C macro:`ACO_USE_VALGRIND`

If you want to use the tool memcheck of valgrind to test the application, then you may need to define the global C macro `ACO_USE_VALGRIND` to enable the friendly support of valgrind in libaco. But it is not recommended to define this macro in the final release build for the performance reason. You may also need to install the valgrind headers (package name is "valgrind-devel" in centos for example) to build libaco application with C macro `ACO_USE_VALGRIND` defined. (The memcheck of valgrind only works well with the standalone co currently. In the case of the shared stack used by more than one non-main co, the memcheck of valgrind would generate many false positive reports. For more information you may refer to "[test_aco_tutorial_6.c](test_aco_tutorial_6.c)".)

## Build

```bash
$ mkdir output
$ bash make.sh
```

## Test

```bash
$ cd output
$ bash ../test.sh
```

# Tutorials

The `test_aco_tutorial_0.c` in this repository shows the basic usgae of libaco. There is only one main co and one standalone non-main co in this tutorial. The comments in the source code is also very helpful.

The `test_aco_tutorial_1.c` shows the usage of some statistics of non-main co. The data structure of `aco_t` is very clear and is defined in `aco.h`.

There are one main co, one standalone non-main co and two non-main co (pointing to the same share stack) in `test_aco_tutorial_2.c`.

The `test_aco_tutorial_3.c` shows how to use libaco in a multithreaded process. Basically, one instance of libaco is designed only to work inside one certain thread to gain the maximum performance of context switching between coroutines. If you want to use libaco in multithreaded environment, simply to create one instance of libaco in each of the threads. There is no data-sharing across threads inside the libaco, and you have to deal with the data competition among multiple threads yourself (like what `gl_race_aco_yield_ct` does in this tutorial).

One of the rules in libaco is to call `aco_exit()` to terminate the execution of the non-main co instead of the default direct C style `return`, otherwise libaco will treat such behaviour as illegal and trigger the default protector whose job is to log the error information about the offending co to stderr and abort the process immediately. The `test_aco_tutorial_4.c` shows such "offending co" situation.

You could also define your own protector to substitute the default one (to do some customized "last words" stuff). But no matter in what case, the process will be aborted after the protector was executed. The `test_aco_tutorial_5.c` shows how to define the customized last word function.

The last example is a simple coroutine scheduler in `test_aco_tutorial_6.c`.

# API

It would be very helpful to read the corresponding API implementation in the source code simultaneously when you are reading the following API description of libaco since the source code is pretty clear and easy to understand. And it is also recommended to read all the [tutorials](#tutorials) before reading the API document.

## aco_thread_init

```c
typedef void (*aco_cofuncp_t)(void);
void aco_thread_init(aco_cofuncp_t last_word_co_fp);
```

To initialize the libaco environment in the current thread.

It will store the current control words of FPU and MXCSR into a thread-local global variable. 

* If the global macro `ACO_CONFIG_SHARE_FPU_MXCSR_ENV` is not defined, the saved control words would be used as a reference value to set up the control words of the new co's FPU and MXCSR (in `aco_create`) and each co would maintain its own copy of FPU and MXCSR control words during later context switching.
* If the global macro `ACO_CONFIG_SHARE_FPU_MXCSR_ENV` is defined, then all the co shares the same control words of FPU and MXCSR. You may refer the "[Build and Test](#build-and-test)" part of this document for more information about this.

And as it said in the `test_aco_tutorial_5.c` of the "[Tutorials](#tutorials)" part, when the 1st argument `last_word_co_fp` is not NULL then the function pointed by `last_word_co_fp` will substitute the default protector to do some "last words" stuff about the offending co before the process is aborted. In such last word function, you could use `aco_get_co` to get the pointer of the offending co. For more information, you may read `test_aco_tutorial_5.c`.

## aco_share_stack_new

```c
aco_share_stack_t* aco_share_stack_new(size_t sz);
```

Equal to `aco_share_stack_new2(sz, 1)`.

## aco_share_stack_new2

```c
aco_share_stack_t* aco_share_stack_new2(size_t sz, char guard_page_enabled);
```

Create a new share stack with a advisory memory size of `sz` in bytes and may have a guard page (read-only) for the detection of stack overflow which is depending on the 2nd argument `guard_page_enabled`.

To use the default size value (2MB) if the 1st argument `sz` equals 0. After some computation of alignment and reserve, this function will ensure the final valid length of the share stack in return:

* `final_valid_sz >= 4096`
* `final_valid_sz >= sz`
* `final_valid_sz % page_size == 0 if the guard_page_enabled == 0`

And as close to the value of `sz` as possible.

When the value of the 2nd argument `guard_page_enabled` is 1, the share stack in return would have one read-only guard page for the detection of stack overflow while a value 0 of `guard_page_enabled` means without such guard page.

This function will always return a valid share stack.

## aco_share_stack_destroy

```c
void aco_share_stack_destroy(aco_share_stack_t* sstk);
```

Destory the share stack `sstk`.

Be sure that all the co whose share stack is `sstk` is already destroyed when you destroy the `sstk`.

## aco_create

```c
typedef void (*aco_cofuncp_t)(void);
aco_t* aco_create(aco_t* main_co，aco_share_stack_t* share_stack, 
        size_t save_stack_sz, aco_cofuncp_t co_fp, void* arg);
```

Create a new co.

If it is a main_co you want to create, just call: `aco_create(NULL, NULL, 0, NULL, NULL)`. Main co is a special standalone coroutine whose share stack is the default thread stack. In the thread, main co is the coroutine who should be created and started to execute before all the other non-main coroutine does.

Otherwise it is a non-main co you want to create:

* The 1st argument `main_co` is the main co the co will `aco_yield` to in the future context switching. `main_co` must not be NULL;
* The 2nd argument `share_stack` is the address of a share stack which the non-main co you want to create will use as its executing stack in the future. `share_stack` must not be NULL;
* The 3rd argument `save_stack_sz` specifies the init size of the private save stack of this co. The unit is in bytes. A value of 0 means to use the default size 64 bytes. Since automatical resizing would happen when the private save stack is not big enough to hold the executing stack of the co when it has to yield the share stack it is occupying to another co, you usually should not worry about the value of `sz` at all. But it will bring some performance impact to the memory allocator when a huge amount (say 10,000,000) of the co resizes their private save stack continuously, so it is very wise and highly recommended to set the `save_stack_sz` with a value equal to the maximum value of `co->save_stack.max_cpsz` when the co is running (You may refer to the "[Best Practice](#best-practice)" part of this document for more information about such optimization);
* The 4th argument `co_fp` is the entry function pointer of the co. `co_fp` must not be NULL;
* The last argument `arg` is a pointer value and will set to `co->arg` of the co to create. It could be used as a input argument for the co.

This function will always return a valid co. And we name the state of the co in return as "init" if it is a non-main co you want to create.

## aco_resume

```c
void aco_resume(aco_t* co);
```

Yield from the caller main co and to start or continue the execution of `co`.

The caller of this function must be a main co and must be `co->main_co`. And the 1st argument `co` must be a non-main co.

The first time you resume a `co`, it starts running the function pointing by `co->fp`. If `co` has already been yielded, `aco_resume` restarts it and continues the execution.

After the call of `aco_resume`, we name the state of the caller — main co as "yielded". 

## aco_yield

```c
void aco_yield();
```

Yield the execution of `co` and resume `co->main_co`. The caller of this function must be a non-main co. And `co->main_co` must not be NULL.

After the call of `aco_yield`, we name the state of the caller — `co` as "yielded".

## aco_get_co

```c
aco_t* aco_get_co();
```

Return the pointer of the current non-main co. The caller of this function must be a non-main co.

## aco_get_arg

```c
void* aco_get_arg();
```

Equal to `(aco_get_co()->arg)`. And also, the caller of this function must be a non-main co.

## aco_exit

```c
void aco_exit();
```

In addition do the same as `aco_yield()`, `aco_exit()` also set `co->is_end` to 1 thus to mark the `co` at the status of "end".

## aco_destroy

```c
void aco_destroy(aco_t* co);
```

Destroy the `co`. The argument `co` must not be NULL. The private save stack would also been destroyed if the `co` is a non-main co.

# Benchmark

```

```

# Proof of Correctness

That is essential to be very familiar with the standard of [Sys V ABI of intel386 and x86-64](https://github.com/hjl-tools/x86-psABI/wiki/X86-psABI) before you start to implement or prove a coroutine library.

This proof below has no direct description about the IP (instruction pointer), SP (stack pointer) and the saving/restoring between the private save stack and the share stack since these things are pretty trivial and easy understanding when they are comparing with the ABI constraints stuff.

## Running Model

In the OS thread, the main coroutine `main_co` is the one who should be created and started to execute before all the other non-main coroutine does.

The next diagram is a simple example of the context switching between main_co and co.

In this proof, we just assume that we are under Sys V ABI of intel386 since there is no fundamental difference between the Sys V ABI of intel386 and x86-64. We also assume that none of the code would change the control words of FPU and MXCSR.

![proof_0](img/proof_0.png)

The next diagram is actually a symmetric coroutine's running model which has unlimited number of non-main co and one main co. This is fine because the asymmetric coroutine is just a special case of the symmetric coroutine. To prove the correctness of the symmetric coroutine is a little more challenging than asymmetric coroutine and thus more fun it would become. (libaco only implemented the API of asymmetric coroutine currently because the semantic meaning of the asymmetric coroutine API is far more easy to understand and to use than the symmetric coroutine does.)

![proof_1](img/proof_1.png)

Since the main co is the 1st coroutine starts to run, the 1st context switching in this OS thread must be in the form of `acosw(main_co, co)` where the 2nd argument `co` is a non-main co.

## Mathematical Induction

It is easy to prove that there only exists two kinds of state transfer in the above diagram:

* yielded state co → init state co
* yielded state co → yielded state co

To prove the correctness of `void* acosw(aco_t* from_co, aco_t* to_co)` implementation is equivalent to prove all the co constantly comply to the constraints of Sys V ABI before and after the call of `acosw`. We assume that the other part of binary code (except `acosw`) in the co had already comply to the ABI (they are normally generated by the compiler correctly).

Here is a summary of the registers' constraints in the Function Calling Convention of Intel386 Sys V ABI:

```
Registers' usage in the calling convention of the Intel386 System V ABI:
    caller saved (scratch) registers:
        C1.0: EAX
            At the entry of a function all:
                could be any value
            After the return of `acosw`:
                hold the return value for `acosw`
        C1.1: ECX,EDX
            At the entry of a function all:
                could be any value
            After the return of `acosw`:
                could be any value
        C1.2: Arithmetic flags, x87 and mxcsr flags
            At the entry of a function all:
                could be any value
            After the return of `acosw`:
                could be any value
        C1.3: ST(0-7)
            At the entry of a function all:
                the stack of FPU must be empty
            After the return of `acosw`:
                the stack of FPU must be empty
        C1.4: Direction flag
            At the entry of a function all:
                DF must be 0
            After the return of `acosw`:
                DF must be 0
        C1.5: others: xmm*,ymm*,mm*,k*...
            At the entry of a function all:
                could be any value
            After the return of `acosw`:
                could be any value
    callee saved registers:
        C2.0: EBX,ESI,EDI,EBP
            At the entry of a function all:
                could be any value
            After the return of `acosw`:
                must be the same as it is at the entry of `acosw` 
        C2.1: ESP
            At the entry of a function all:
                must be a valid stack pointer
                (alignment of 16 bytes, retaddr and etc...)
            After the return of `acosw`:
                must be the same as it is before the call of `acosw`
        C2.2: control word of FPU & mxcsr
            At the entry of a function all:
                could be any configuration
            After the return of `acosw`:
                must be the same as it is before the call of `acosw` 
                (unless the caller of `acosw` assume `acosw` may    \
                change the control words of FPU or MXCSR on purpose \
                like `fesetenv`)
```

**Proof:**

1. yielded state co -> init state co:

![proof_2](img/proof_2.png)

The diagram above is for the 1st case: "yielded state co -> init state co".

Constraints: C 1.0, 1.1, 1.2, 1.5 (*satisfied* √ )

The scratch registers below can hold any value at the entry of a function:

```
EAX,ECX,EDX
XMM*,YMM*,MM*,K*...
status bits of EFLAGS,FPU,MXCSR
```

Constraints: C 1.3, 1.4 (*satisfied* √ )

Since the stack of FPU must already be empty and the DF must already be 0 before `acosw(co, to_co)` was called (the binary code of co is already complied to the ABI), the constraint 1.3 and 1.4 is complied by `acosw`.

Constraints: C 2.0, 2.1, 2.2 (*satisfied* √ )

C 2.0 & 2.1 is already satisfied. Since we already assumed that nobody will change the control words of FPU and MXCSR, C 2.2 is satisfied too.

2. yielded state co -> yielded state co:

![proof_3](img/proof_3.png)

The diagram above is for the 2nd case: yielded state co -> yielded state co.

Constraints: C 1.0 (*satisfied* √ )

EAX already holding the return value when `acosw` returns back to to_co (resume).

Constraints: C 1.1, 1.2, 1.5 (*satisfied* √ )

The scratch registers below can hold any value at the entry of a function and after the return of `acosw`:

```
ECX,EDX
XMM*,YMM*,MM*,K*...
status bits of EFLAGS,FPU,MXCSR
```

Constraints: C 1.3, 1.4 (*satisfied* √ )

Since the stack of FPU must already be empty and the DF must already be 0 before `acosw(co, to_co)` was called (the binary code of co is already complied to the ABI), the constraint 1.3 and 1.4 is complied by `acosw`.

Constraints: C 2.0, 2.1, 2.2 (*satisfied* √ )

C 2.0 & 2.1 is satisfied because there is saving & restoring of the callee saved registers when `acosw` been called/returned. Since we already assumed that nobody will change the control words of FPU and MXCSR, C 2.2 is satisfied too.

3. mathematical induction:

The 1st `acosw` in the thread must be the 1st case: yielded state co -> init state co, and all the next `acosw` must be one of the 2 case above. Sequentially, we could prove that "all the co constantly comply to the constraints of Sys V ABI before and after the call of `acosw`". Thus, the proof is finished.

## Miscellaneous

### Red Zone

There is a new thing called [red zone](https://en.wikipedia.org/wiki/Red_zone_(computing)) in System V ABI x86-64:

> The 128-byte area beyond the location pointed to by %rsp is considered to be reserved and shall not be modified by signal or interrupt handlers. Therefore, functions may use this area for temporary data that is not needed across function calls. In particular, leaf functions may use this area for their entire stack frame, rather than adjusting the stack pointer in the prologue and epilogue. This area is known as the red zone.

Since the red zone is "not preserved by the callee", we just do not care about it at all in the context switching between coroutines (because the `acosw` is a leaf function).

### Stack Pointer

> The end of the input argument area shall be aligned on a 16 (32 or 64, if \_\_m256 or \_\_m512 is passed on stack) byte boundary. In other words, the value (%esp + 4) is always a multiple of 16 (32 or 64) when control is transferred to the function entry point. The stack pointer, %esp, always points to the end of the latest allocated stack frame.
>
> — Intel386-psABI-1.1:2.2.2 The Stack Frame

> The stack pointer, %rsp, always points to the end of the latest allocated stack frame.
>
> — Sys V ABI AMD64 Version 1.0:3.2.2 The Stack Frame

Here is a [bug example](https://github.com/Tencent/libco/blob/v1.0/coctx_swap.S#L27) in Tencent's libco. The ABI states that the `(E|R)SP` should always point to the end of the latest allocated stack frame. But in file [coctx_swap.S](https://github.com/Tencent/libco/blob/v1.0/coctx_swap.S#L27) of libco, the `(E|R)SP` had been used to address the memory on the heap.

>**By default, the signal handler  is invoked  on  the normal process stack.**  It is possible to arrange that the signal handler uses an alternate stack; see sigalstack(2)  for  a discussion of how to do this and when it might be useful.
>
>— man 7 signal : Signal dispositions

Terrible things may happend if the `(E|R)SP`  is pointing to the data structure on the heap when signal comes. (Using the `breakpoint` and `signal` commands of gdb could produce such bug conveniently. Although by using `sigalstack` to change the default signal stack could alleviate the problem, but still, that kind of usage of `(E|R)SP` still violates the ABI.)

# Best Practice

In summary, if you want to gain the ultra performance of libaco, just keep the stack usage of the non-standalone non-main co at the point of calling `aco_yield` as small as possible. And be very careful if you want to pass the address of a local variable from one co to another co since the local variable is usually on the **share** stack. Allocating this kind of variables from the heap is always the wiser choice.

In detail, there are 5 tips:

```
       co_fp 
       /  \
      /    \  
    f1     f2
   /  \    / \
  /    \  f4  \
yield  f3     f5
```

1. The stack usage of main co when it is been yielded (i.e. call `aco_resume` to resume other non-main co) has no direct influence to the performance of context switching between coroutines (since it has a standalone execution stack);
2. The stack usage of standalone non-main co when it is been yielded (i.e. call `aco_yield` to yield back to main co) has no direct influence to the performance of context switching between coroutines. But a huge amount of standalone non-main co would cost too much of virtual memory (due to the standalone stack), so it is not recommended to create huge amount of standalone non-main co in one thread;
3. The stack usage of non-standalone (share stack with other coroutines) non-main co when it is been yielded (i.e. call `aco_yield` to yield back to main co) has big impact to the performance of context switching between coroutines. The benchmark result shows that clearly already. In the diagram above, the stack usage of function f2, f3, f4 and f5 has no direct influence to context switching performance since there are no `aco_yield` when they are executing. Whereas the stack usage of co_fp and f1 dominates the value of `co->save_stack.max_cpsz` and has a big influence to the context switching performance.

The key to keep a tiny stack usage of a function is to allocate the local variables (especially the big ones) on the heap and manage their lifecycle manually instead of allocating them on the stack by default. The `-fstack-usage` option of gcc is very helpful about this.

```c
int* gl_ptr;

void inc_p(int* p){ (*p)++; }

void co_fp0() {
    int ct = 0;
    gl_ptr = &ct; // line 7
    aco_yield();
    check(ct);
    int* ptr = &ct;
    inc_p(ptr);
    aco_exit();
}

void co_fp1() {
    do_sth(gl_ptr); // line 16
    aco_exit();
}
```

4. In the above code snippet, we assume that co_fp0 & co_fp1 shares the same share stack (they are both non-main co) and the running sequence of them is "co_fp0 -> co_fp1 -> co_fp0". Since they are sharing the same stack, the address holding in `gl_ptr` in co_fp1 (line 16) has totally different semantics with the `gl_ptr` in line 7 of co_fp0, and that kind of code would probably corrupt the execution stack of co_fp1. But the line 11 is fine because variable `ct` and function `inc_p` are in the same coroutine context. Allocating that kind of variables (need to share with other coroutines) on the heap would simply solve such problems:

```c
int* gl_ptr;

void inc_p(int* p){ (*p)++; }

void co_fp0() {
    int* ct_ptr = malloc(sizeof(int));
    assert(ct_ptr != NULL);
    *ct_ptr = 0;
    gl_ptr = ct_ptr;
    aco_yield();
    check(*ct_ptr);
    int* ptr = ct_ptr;
    inc_p(ptr);
    free(ct_ptr);
    gl_ptr = NULL;
    aco_exit();
}

void co_fp1() {
    do_sth(gl_ptr);
    aco_exit();
}
```

# Donation

I'm a full-time open source developer. Any amount of the donations will be highly appreciated and could bring me great encouragement.

* Paypal

  [paypal.me link](https://www.paypal.me/00hnes)

* Alipay (支付(宝|寶))

![qr_alipay](img/qr_alipay.png)

* Wechat (微信)

![qr_wechat](img/qr_wechat.png)

# Copyright and License

Copyright (C) 2018, by Sen Han [00hnes@gmail.com](mailto:00hnes@gmail.com).

Under the Apache License, Version 2.0.

See the [LICENSE](LICENSE) file for details.
