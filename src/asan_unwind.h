#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

namespace asan_unwind {

class StackTrace
{
public:
    StackTrace(void* frames, size_t maxFrames);

    size_t unwind();

private:
    void unwind_internal();

    void* mFrames;
    size_t mMaxFrames;
    size_t mFrameCount {};
};

inline StackTrace::StackTrace(void* frames, size_t maxFrames)
    : mFrames(frames), mMaxFrames(maxFrames)
{
}

inline size_t StackTrace::unwind()
{
    if (!mFrameCount)
        unwind_internal();
    return mFrameCount;
}

} // namespace asan_unwind
