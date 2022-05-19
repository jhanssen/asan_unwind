//===-- sanitizer_stacktrace.cpp ------------------------------------------===//
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

#include "sanitizer_stacktrace.h"
#include <cassert>

inline constexpr bool IsAligned(uintptr_t a, uintptr_t alignment) {
    return (a & (alignment - 1)) == 0;
}

namespace __sanitizer {

uintptr_t StackTrace::GetNextInstructionPc(uintptr_t pc) {
#if defined(__arm__)
    return pc + 4;
#elif defined(__sparc__) || defined(__mips__)
    return pc + 8;
#else
    return pc + 1;
#endif
}

uintptr_t StackTrace::GetCurrentPc() {
    return GET_CALLER_PC();
}

// In GCC on ARM bp points to saved lr, not fp, so we should check the next
// cell in stack to be a saved frame pointer. GetCanonicFrame returns the
// pointer to saved frame pointer in any case.
static inline uhwptr *GetCanonicFrame(uintptr_t bp,
                                      uintptr_t stack_top,
                                      uintptr_t stack_bottom) {
    assert(stack_top > stack_bottom);
#ifdef __arm__
    if (!IsValidFrame(bp, stack_top, stack_bottom)) return 0;
    uhwptr *bp_prev = (uhwptr *)bp;
    if (IsValidFrame((uintptr_t)bp_prev[0], stack_top, stack_bottom)) return bp_prev;
    // The next frame pointer does not look right. This could be a GCC frame, step
    // back by 1 word and try again.
    if (IsValidFrame((uintptr_t)bp_prev[-1], stack_top, stack_bottom))
        return bp_prev - 1;
    // Nope, this does not look right either. This means the frame after next does
    // not have a valid frame pointer, but we can still extract the caller PC.
    // Unfortunately, there is no way to decide between GCC and LLVM frame
    // layouts. Assume GCC.
    return bp_prev - 1;
#else
    return (uhwptr*)bp;
#endif
}

void FastStackTrace::UnwindFast(uintptr_t pc, uintptr_t bp, uintptr_t stack_top,
                                uintptr_t stack_bottom, uint32_t max_depth) {
    // TODO(yln): add arg sanity check for stack_top/stack_bottom
    assert(max_depth > 2);
    const uintptr_t kPageSize = GetPageSizeCached();
    trace[0] = pc;
    size = 1;
    if (stack_top < 4096) return;  // Sanity check for stack top.
    uhwptr *frame = GetCanonicFrame(bp, stack_top, stack_bottom);
    // Lowest possible address that makes sense as the next frame pointer.
    // Goes up as we walk the stack.
    uintptr_t bottom = stack_bottom;
    // Avoid infinite loop when frame == frame[0] by using frame > prev_frame.
    while (IsValidFrame((uintptr_t)frame, stack_top, bottom) &&
           IsAligned((uintptr_t)frame, sizeof(*frame)) &&
           size < max_depth) {
#ifdef __powerpc__
        // PowerPC ABIs specify that the return address is saved on the
        // *caller's* stack frame.  Thus we must dereference the back chain
        // to find the caller frame before extracting it.
        uhwptr *caller_frame = (uhwptr*)frame[0];
        if (!IsValidFrame((uintptr_t)caller_frame, stack_top, bottom) ||
            !IsAligned((uintptr_t)caller_frame, sizeof(uhwptr)))
            break;
        // For most ABIs the offset where the return address is saved is two
        // register sizes.  The exception is the SVR4 ABI, which uses an
        // offset of only one register size.
        uhwptr pc1 = caller_frame[2];
#elif defined(__s390__)
        uhwptr pc1 = frame[14];
#elif defined(__riscv)
        // frame[-1] contains the return address
        uhwptr pc1 = frame[-1];
#else
        uhwptr pc1 = frame[1];
#endif
        // Let's assume that any pointer in the 0th page (i.e. <0x1000 on i386 and
        // x86_64) is invalid and stop unwinding here.  If we're adding support for
        // a platform where this isn't true, we need to reconsider this check.
        if (pc1 < kPageSize)
            break;
        if (pc1 != pc) {
            trace[size++] = (uintptr_t) pc1;
        }
        bottom = (uintptr_t)frame;
#if defined(__riscv)
        // frame[-2] contain fp of the previous frame
        uintptr_t new_bp = (uintptr_t)frame[-2];
#else
        uintptr_t new_bp = (uintptr_t)frame[0];
#endif
        frame = GetCanonicFrame(new_bp, stack_top, bottom);
    }
}

void FastStackTrace::PopStackFrames(uintptr_t count) {
    assert(count < size);
    size -= count;
    for (uintptr_t i = 0; i < size; ++i) {
        trace[i] = trace[i + count];
    }
}

static uintptr_t Distance(uintptr_t a, uintptr_t b) { return a < b ? b - a : a - b; }

uintptr_t FastStackTrace::LocatePcInTrace(uintptr_t pc) {
    uintptr_t best = 0;
    for (uintptr_t i = 1; i < size; ++i) {
        if (Distance(trace[i], pc) < Distance(trace[best], pc)) best = i;
    }
    return best;
}

} // namespace __sanitizer
