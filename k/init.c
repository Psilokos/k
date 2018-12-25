#include <assert.h>
#include <k/types.h>

static struct __attribute__((packed))
{
    unsigned limit_low : 16;
    unsigned base_low : 24;
    unsigned accessed : 1;
    unsigned read_write : 1;
    unsigned direction_conforming : 1;
    unsigned executable : 1;
    enum { SYSTEM, CODE_DATA } type : 1;
    unsigned dpl : 2;
    unsigned present : 1;
    unsigned limit_high : 4;
    unsigned reserved : 2;
    unsigned size : 1;
    unsigned granularity : 1;
    unsigned base_high : 8;
} gdt[] =
{
    [1] = /* code */
    {
        .base_low = 0,
        .base_high = 0,
        .limit_low = 0xffff,
        .limit_high = 0xf,
        .accessed = 0,
        .read_write = 0,
        .direction_conforming = 0,
        .executable = 1,
        .type = CODE_DATA,
        .dpl = 0,
        .present = 1,
        .reserved = 0,
        .size = 1,
        .granularity = 1
    },
    [2] = /* data */
    {
        .base_low = 0,
        .base_high = 0,
        .limit_low = 0xffff,
        .limit_high = 0xf,
        .accessed = 0,
        .read_write = 1,
        .direction_conforming = 0,
        .executable = 0,
        .type = CODE_DATA,
        .dpl = 0,
        .present = 1,
        .reserved = 0,
        .size = 1,
        .granularity = 1
    }
};
_Static_assert(sizeof(*gdt) == 8,
               "segment descriptor layout mismatch");

static struct __attribute__((packed))
{
    u16 limit;
    void *base;
} gdtr =
{
    .limit = sizeof(gdt),
    .base = gdt
};
_Static_assert(sizeof(gdtr) == 6,
               "GDT register layout mismatch");

static inline void
switch_to_protected(void)
{
    __asm__ volatile ("lgdt %[gdtr]\n"
                      "mov  %%cr0, %%eax\n"
                      "or      $1, %%eax\n"
                      "mov  %%eax, %%cr0\n"
                      "mov  %[ds], %%ax\n"
                      "mov   %%ax, %%ds\n"
                      "mov   %%ax, %%es\n"
                      "mov   %%ax, %%fs\n"
                      "mov   %%ax, %%gs\n"
                      "ljmp %[cs], $set_cs\n"
                      "set_cs:\n"
                      :
                      : [gdtr]"m"(gdtr), [cs]"i"(0x8), [ds]"i"(0x10)
                      : "%eax");
}

void
init(void)
{
    switch_to_protected();
}
