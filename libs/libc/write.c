#include <stddef.h>
#include <k/types.h>

int
write(char const *buf, size_t sz)
{
    u16 port = 0x3F8;
    for (size_t i = 0; i < sz; ++i)
        __asm__ volatile ("out %[val], %[port]\n"
                          :: [port]"d"(port), [val]"a"(buf[i]));
    return sz;
}
