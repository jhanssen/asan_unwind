#include "asan_unwind.h"
#include "sanitizer_stacktrace.h"
#include <cstring>
#include <cstdio>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>

using namespace asan_unwind;

struct StackLimits
{
    void* stackaddr {};
    size_t stacksize {};

    void init();
};

inline void StackLimits::init()
{
    pthread_attr_t attr;
    int i = pthread_getattr_np(pthread_self(), &attr);
    if (i != 0)
        return;
    i = pthread_attr_getstack(&attr, &stackaddr, &stacksize);
    if (i != 0)
        return;
    pthread_attr_destroy(&attr);
}

thread_local StackLimits stackLimits;

void StackTrace::unwind_internal()
{
    if (stackLimits.stackaddr == nullptr)
        stackLimits.init();

    uintptr_t stackbottom = reinterpret_cast<uintptr_t>(stackLimits.stackaddr);
    uintptr_t stacktop = stackbottom + stackLimits.stacksize;

    uintptr_t pc = GET_CALLER_PC();
    uintptr_t bp = GET_CURRENT_FRAME();

    // printf("blblblb %lx %lx %lx %lx max %zu\n", stackbottom, stacktop, pc, bp, mMaxFrames);

    static_assert(sizeof(uintptr_t) == sizeof(void*), "sizeof(uintptr_t) needs to be the same as sizeof(void*)");
    __sanitizer::FastStackTrace bst(reinterpret_cast<uintptr_t*>(mFrames));
    bst.UnwindFast(pc, bp, stacktop, stackbottom, mMaxFrames);
    mFrameCount = bst.size;
}
