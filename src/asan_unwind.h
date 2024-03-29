#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

namespace asan_unwind {

class StackTrace
{
public:
    StackTrace(void* frames, size_t maxFrames);

    size_t unwindFast(unsigned skip);
    size_t unwindSlow(unsigned skip);

private:
    void* mFrames;
    size_t mMaxFrames;
};

inline StackTrace::StackTrace(void* frames, size_t maxFrames)
    : mFrames(frames), mMaxFrames(maxFrames)
{
}

} // namespace asan_unwind
