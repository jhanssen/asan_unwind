//===-- sanitizer_stacktrace.h ----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is shared between AddressSanitizer and ThreadSanitizer
// run-time libraries.
//===----------------------------------------------------------------------===//
#ifndef SANITIZER_STACKTRACE_H
#define SANITIZER_STACKTRACE_H

#include <cstdint>

#if defined(__x86_64__)
// Since x32 uses ILP32 data model in 64-bit hardware mode, we must use
// 64-bit pointer to unwind stack frame.
typedef unsigned long long uhwptr;
#else
typedef uintptr_t uhwptr;
#endif

namespace __sanitizer {

#if defined(__mips__)
# define SANITIZER_CAN_FAST_UNWIND 0
#else
# define SANITIZER_CAN_FAST_UNWIND 1
#endif

// Fast unwind is the only option on Mac for now; we will need to
// revisit this macro when slow unwind works on Mac, see
// https://github.com/google/sanitizers/issues/137
#if defined(__APPLE__)
#  define SANITIZER_CAN_SLOW_UNWIND 0
#else
# define SANITIZER_CAN_SLOW_UNWIND 1
#endif

struct StackTrace {
    uintptr_t *trace;
    uint32_t size;
    uint32_t tag;

    static const int TAG_UNKNOWN = 0;
    static const int TAG_ALLOC = 1;
    static const int TAG_DEALLOC = 2;
    static const int TAG_CUSTOM = 100; // Tool specific tags start here.

    StackTrace() : trace(nullptr), size(0), tag(0) {}
    StackTrace(uintptr_t *trace, uint32_t size) : trace(trace), size(size), tag(0) {}
    StackTrace(uintptr_t *trace, uint32_t size, uint32_t tag)
        : trace(trace), size(size), tag(tag) {}

    static bool WillUseFastUnwind(bool request_fast_unwind) {
        if (!SANITIZER_CAN_FAST_UNWIND)
            return false;
        if (!SANITIZER_CAN_SLOW_UNWIND)
            return true;
        return request_fast_unwind;
    }

    static uintptr_t GetCurrentPc();
    static inline uintptr_t GetPreviousInstructionPc(uintptr_t pc);
    static uintptr_t GetNextInstructionPc(uintptr_t pc);
};

// Performance-critical, must be in the header.
inline __attribute__((always_inline))
uintptr_t StackTrace::GetPreviousInstructionPc(uintptr_t pc) {
#if defined(__arm__)
    // T32 (Thumb) branch instructions might be 16 or 32 bit long,
    // so we return (pc-2) in that case in order to be safe.
    // For A32 mode we return (pc-4) because all instructions are 32 bit long.
    return (pc - 3) & (~1);
#elif defined(__sparc__) || defined(__mips__)
    return pc - 8;
#else
    return pc - 1;
#endif
}

struct FastStackTrace : public StackTrace {
    FastStackTrace(uintptr_t* buffer) : StackTrace(buffer, 0) {}

    void UnwindFast(uintptr_t pc, uintptr_t bp, uintptr_t stack_top, uintptr_t stack_bottom,
                    uint32_t max_depth);

private:
    void PopStackFrames(uintptr_t count);
    uintptr_t LocatePcInTrace(uintptr_t pc);

    FastStackTrace(const FastStackTrace &) = delete;
    void operator=(const FastStackTrace &) = delete;

    friend class FastUnwindTest;
};

static const uintptr_t kFrameSize = 2 * sizeof(uhwptr);

// Check if given pointer points into allocated stack area.
static inline bool IsValidFrame(uintptr_t frame, uintptr_t stack_top, uintptr_t stack_bottom) {
    return frame > stack_bottom && frame < stack_top - kFrameSize;
}

}  // namespace __sanitizer

#define GET_CALLER_PC() (uintptr_t) __builtin_return_address(0)
#define GET_CURRENT_FRAME() (uintptr_t) __builtin_frame_address(0)

// Use this macro if you want to print stack trace with the caller
// of the current function in the top frame.
#define GET_CALLER_PC_BP                        \
    uintptr_t bp = GET_CURRENT_FRAME();         \
    uintptr_t pc = GET_CALLER_PC();

#define GET_CALLER_PC_BP_SP                     \
    GET_CALLER_PC_BP;                           \
    uintptr_t local_stack;                      \
    uintptr_t sp = (uintptr_t)&local_stack

// Use this macro if you want to print stack trace with the current
// function in the top frame.
#define GET_CURRENT_PC_BP                       \
    uintptr_t bp = GET_CURRENT_FRAME();         \
    uintptr_t pc = StackTrace::GetCurrentPc()

#define GET_CURRENT_PC_BP_SP                    \
    GET_CURRENT_PC_BP;                          \
    uintptr_t local_stack;                      \
    uintptr_t sp = (uintptr_t)&local_stack

// GET_CURRENT_PC() is equivalent to StackTrace::GetCurrentPc().
// Optimized x86 version is faster than GetCurrentPc because
// it does not involve a function call, instead it reads RIP register.
// Reads of RIP by an instruction return RIP pointing to the next
// instruction, which is exactly what we want here, thus 0 offset.
// It needs to be a macro because otherwise we will get the name
// of this function on the top of most stacks. Attribute artificial
// does not do what it claims to do, unfortunatley. And attribute
// __nodebug__ is clang-only. If we would have an attribute that
// would remove this function from debug info, we could simply make
// StackTrace::GetCurrentPc() faster.
#if defined(__x86_64__)
#  define GET_CURRENT_PC()                      \
    (__extension__({                            \
            uintptr_t pc;                        \
            asm("lea 0(%%rip), %0" : "=r"(pc)); \
            pc;                                 \
        }))
#else
#  define GET_CURRENT_PC() StackTrace::GetCurrentPc()
#endif

#endif  // SANITIZER_STACKTRACE_H
