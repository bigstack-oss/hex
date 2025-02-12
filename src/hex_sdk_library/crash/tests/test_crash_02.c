
#include <hex/crash.h>
#include <hex/test.h>

#include <libgen.h>
#include <alloca.h>

static
void cb(void* data, void* context)
{
    HEX_TEST(*(int*)data == 13);
}

// n and v don't really mean anything here
static
void crash(int n, int* v)
{
    if (n > 1)
        abort();
}

static
void foo(int n)
{
    // try to avoid getting optimized out
    int* v = alloca(sizeof(int)*n); // foo won't be inlined because of alloca
    int i;
    for (i = 0; i < n; i++)
        v[i] = i;
    crash(n, v);
}

// argc > 1 means this program will abort() on assignment to address 0x13
int main(int argc, char* argv[])
{
    int tmp = 13;
    HEX_TEST(HexCrashInitPrepend(basename(argv[0]), cb, &tmp) == 0);

    printf("%d\n", getpid()); // display pid so wrapper script can look for crash file
    fflush(stdout);
    foo(argc);
    return 0;
}
