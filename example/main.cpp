#include <asan_unwind.h>
#include <cstdio>

static void log(void* ptrs, size_t size)
{
    printf("   %zu frames\n", size);
    for (size_t i = 0; i < size; ++i) {
        printf("%lx\n", reinterpret_cast<uintptr_t*>(ptrs)[i]);
    }
}

void func2()
{
}

void func1()
{
    std::array<void*, 255> mFrames;
    asan_unwind::StackTrace stackTrace(mFrames.data(), 255);
    const auto num = stackTrace.unwind();
    log(mFrames.data(), num);
    //func2();
}

int main()
{
    func1();
}
