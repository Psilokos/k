#include <assert.h>
#include <k/types.h>
#include "io.h"
#include "ps2.h"

static void
init_UART(u16 rate)
{
    u8 lcr = inb(PORT_COM1 + 3);
    outb(PORT_COM1 + 3, lcr | 1 << 7);
    outb(PORT_COM1 + 0, (115200 / rate) & 0xFF);
    outb(PORT_COM1 + 1, (115200 / rate) >> 8);
    outb(PORT_COM1 + 3, lcr & ~(1 << 7));
}

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
    unsigned _reserved : 2;
    unsigned size : 1;
    unsigned granularity : 1;
    unsigned base_high : 8;
} gdt[] =
{
    [0] = { 0 },
    [1] = /* code */
    {
        .base_low = 0,
        .base_high = 0,
        .limit_low = 0XFFFF,
        .limit_high = 0xF,
        .accessed = 0,
        .read_write = 0,
        .direction_conforming = 0,
        .executable = 1,
        .type = CODE_DATA,
        .dpl = 0,
        .present = 1,
        ._reserved = 0,
        .size = 1,
        .granularity = 1
    },
    [2] = /* data */
    {
        .base_low = 0,
        .base_high = 0,
        .limit_low = 0xFFFF,
        .limit_high = 0xF,
        .accessed = 0,
        .read_write = 1,
        .direction_conforming = 0,
        .executable = 0,
        .type = CODE_DATA,
        .dpl = 0,
        .present = 1,
        ._reserved = 0,
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

static struct __attribute__((packed))
{
    u16 offset_low;
    u16 selector;
    u8  zero;
    u8  type;
    u16 offset_high;
} idt[0x100];
_Static_assert(sizeof(*idt) == 8,
               "gate descriptor layout mismatch");

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

enum gate_type
{
    INTERRUPT_GATE  = 0x8E,
    TRAP_GATE       = 0x8F,
};

static inline void
load_IDT(void)
{
    extern void isr_0(void);
    extern void isr_1(void);
    extern void isr(void);

    u8 delta = &isr_1 - &isr_0;

    u32 isr_addr = (u32)&isr_0;

    for (int i = 0; i < 0x30; ++i)
    {
        idt[i] = (typeof(*idt))
        {
            .offset_low  = isr_addr & 0xFFFF,
            .offset_high = isr_addr >> 16,
            .selector    = 0x8,
            .zero        = 0,
            .type        = i == 3 ? TRAP_GATE : INTERRUPT_GATE
        };
        if (delta == 4 && isr_addr + delta - (u32)&isr > 127)
            delta += 3;
        isr_addr += delta;
    }

    struct __attribute__((packed))
    {
        u16 limit;
        void *base;
    } idtr =
    {
        .limit = 0x30 * sizeof(*idt) - 1,
        .base = idt
    };
    _Static_assert(sizeof(idtr) == 6,
                   "IST register layout mismatch");
    __asm__ volatile ("lidt %[idtr]\n" :: [idtr]"m"(idtr));
}

static inline void
remap_PIC(void)
{
    /* store state */
    u8 master = inb(PORT_PIC_MASTER_DATA);
    u8 slave = inb(PORT_PIC_SLAVE_DATA);

    /* ICW1 */
    outb(PORT_PIC_MASTER_CMD, 0x11);
    outb(PORT_PIC_SLAVE_CMD, 0x11);
    /* ICW2 */
    outb(PORT_PIC_MASTER_DATA, 0x20);
    outb(PORT_PIC_SLAVE_DATA, 0x28);
    /* ICW3 */
    outb(PORT_PIC_MASTER_DATA, 1 << 2);
    outb(PORT_PIC_SLAVE_DATA, 2);
    /* ICW4 */
    outb(PORT_PIC_MASTER_DATA, 1);
    outb(PORT_PIC_SLAVE_DATA, 1);

    /* restore state */
    outb(PORT_PIC_MASTER_DATA, master);
    outb(PORT_PIC_SLAVE_DATA, slave);
}

int
init(void)
{
    __asm__ volatile ("cli\n");
    init_UART(38400);
    switch_to_protected();
    load_IDT();
    remap_PIC();
    __asm__ volatile ("sti\n");
    if (ps2_init()) return -1;
    return 0;
}
