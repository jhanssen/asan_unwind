#include <asan_unwind.h>
#include <cstdio>

static void log(const std::pair<const uintptr_t*, size_t>& st)
{
    printf("   %zu frames\n", st.second);
    for (size_t i = 0; i < st.second; ++i) {
        printf("%lx\n", st.first[i]);
    }
}

void func2()
{
}

void func1()
{
    asan_unwind::StackTrace stackTrace;
    log(stackTrace.unwind());
    //func2();
}

int main()
{
    func1();
}
