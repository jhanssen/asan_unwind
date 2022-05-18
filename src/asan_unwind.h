#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

namespace asan_unwind {

class StackTrace
{
public:
    StackTrace() = default;

    std::pair<const uintptr_t*, size_t> unwind();

private:
    void unwind_internal();

    enum { MaxFrames = 255 };

    std::array<uintptr_t, MaxFrames> mFrames;
    size_t mFrameCount {};
};

inline std::pair<const uintptr_t*, size_t> StackTrace::unwind()
{
    if (!mFrameCount)
        unwind_internal();
    return std::make_pair(mFrames.data(), mFrameCount);
}

} // namespace asan_unwind
