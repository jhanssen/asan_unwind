#include "asan_unwind.h"
#include "sanitizer_stacktrace.h"
#include <cstring>
#include <cstdio>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>

using namespace asan_unwind;

void StackTrace::unwind_internal()
{
    pthread_attr_t attr;
    int i = pthread_getattr_np(pthread_self(), &attr);
    if (i != 0)
        return;
    void* stackaddr;
    size_t stacksize;
    i = pthread_attr_getstack(&attr, &stackaddr, &stacksize);
    if (i != 0)
        return;
    pthread_attr_destroy(&attr);

    uintptr_t stackbottom = reinterpret_cast<uintptr_t>(stackaddr);
    uintptr_t stacktop = stackbottom + stacksize;

    uintptr_t pc = GET_CALLER_PC();
    uintptr_t bp = GET_CURRENT_FRAME();

    __sanitizer::FastStackTrace bst(mFrames.data());
    bst.UnwindFast(pc, bp, stacktop, stackbottom, MaxFrames);
    mFrameCount = bst.size;
}
