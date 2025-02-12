
#include <hex/crash.h>
#include <hex/test.h>

#include <libgen.h>
#include <alloca.h>

volatile int* loc; // make volatile to avoid any optimization

// n and v don't really mean anything here
static
void crash(int n, int* v)
{
    if (n > 1)
        loc = (int*)0x13; // set to illegal address to generate crash
    *loc = v[0];
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

// argc > 1 means this program will crash on assignment to address 0x13
int main(int argc, char* argv[])
{
    HEX_TEST(HexCrashInit(basename(argv[0])) == 0);

    int tmp = 0;
    loc = &tmp; // make loc point to a legal address for now
    printf("%d\n", getpid()); // display pid so wrapper script can look for crash file
    fflush(stdout);
    foo(argc);
    return 0;
}
