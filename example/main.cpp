#include <asan_unwind.h>
#include <cstdio>

static void log(void* ptrs, size_t size)
{
    printf("   %zu frames\n", size);
    for (size_t i = 0; i < size; ++i) {
        printf("%llx\n", static_cast<unsigned long long>(reinterpret_cast<uintptr_t*>(ptrs)[i]));
    }
}

void func2()
{
}

void func1()
{
    std::array<void*, 255> mFrames;
    asan_unwind::StackTrace stackTrace(mFrames.data(), 255);
    printf("fast\n");
    auto num = stackTrace.unwindFast(0);
    log(mFrames.data(), num);
    printf("slow\n");
    num = stackTrace.unwindSlow(0);
    log(mFrames.data(), num);
    //func2();
}

int main()
{
    func1();
}
