#include "asan_unwind.h"
// #include "sanitizer_stacktrace.h"
#include <cstring>
#include <cstdio>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>
#include <unwind.h>

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

size_t StackTrace::unwindFast()
{
    return 0;
    // if (stackLimits.stackaddr == nullptr)
    //     stackLimits.init();

    // const uintptr_t pc = GET_CALLER_PC();
    // const uintptr_t bp = GET_CURRENT_FRAME();

    // uintptr_t stackbottom = reinterpret_cast<uintptr_t>(stackLimits.stackaddr);
    // uintptr_t stacktop = stackbottom + stackLimits.stacksize;


    // // printf("blblblb %lx %lx %lx %lx max %zu\n", stackbottom, stacktop, pc, bp, mMaxFrames);

    // static_assert(sizeof(uintptr_t) == sizeof(void*), "sizeof(uintptr_t) needs to be the same as sizeof(void*)");
    // __sanitizer::FastStackTrace bst(reinterpret_cast<uintptr_t*>(mFrames));
    // bst.UnwindFast(pc, bp, stacktop, stackbottom, mMaxFrames);
    // return bst.size;
}

struct UnwindTraceArg {
    uintptr_t* frames;
    size_t numFrames;
    size_t maxFrames;
};

#if defined(__arm__)
#define UNWIND_STOP _URC_END_OF_STACK
#define UNWIND_CONTINUE _URC_NO_REASON
#else
#define UNWIND_STOP _URC_NORMAL_STOP
#define UNWIND_CONTINUE _URC_NO_REASON
#endif

static _Unwind_Reason_Code Unwind_Trace(struct _Unwind_Context *ctx, void *param) {
    UnwindTraceArg *arg = (UnwindTraceArg*)param;
    uintptr_t pc = _Unwind_GetIP(ctx);
    const uintptr_t kPageSize = 4096; //__sanitizer::GetPageSizeCached();
    // Let's assume that any pointer in the 0th page (i.e. <0x1000 on i386 and
    // x86_64) is invalid and stop unwinding here.  If we're adding support for
    // a platform where this isn't true, we need to reconsider this check.
    if (pc < kPageSize) return UNWIND_STOP;
    if (arg->numFrames >= arg->maxFrames) return UNWIND_STOP;
    arg->frames[arg->numFrames++] = pc;
    return UNWIND_CONTINUE;
}

size_t StackTrace::unwindSlow()
{
    UnwindTraceArg arg = { reinterpret_cast<uintptr_t*>(mFrames), 0, mMaxFrames };
    _Unwind_Backtrace(Unwind_Trace, &arg);
    return arg.numFrames;
}
