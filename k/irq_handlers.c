#include <k/types.h>
#include <stdio.h>
#include "io.h"
#include "ps2.h"
#include "timer.h"

typedef void (*isr_fptr)(void);

static inline void
divide_by_zero(void)
{
    puts("divide by zero\n");
}

isr_fptr exception_handlers[] =
{
    &divide_by_zero,
};

static inline void
PIT_handler(void)
{
    timer_tick();
}

static inline void
kbd_handler(void)
{
    if (ps2_kbd_save_keycode(inb(PORT_PS2_DATA)))
        puts("full buf\n");
};

isr_fptr hardint_handlers[] =
{
    &PIT_handler,
    &kbd_handler,
};

isr_fptr softint_handlers[] =
{
};

static inline void
hardint_handler(u32 num)
{
    hardint_handlers[num]();
    if (num > 7)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void
irq_handler(void *ptr)
{
    __asm__ volatile ("cli\n");
    u32 irq_num = *((u32 *)ptr + 8);
    if (irq_num < 0x20)
    {
        printf("irq %u\n", irq_num);
        if (irq_num == 0)
        exception_handlers[irq_num]();
    }
    else if (irq_num < 0x30)
        hardint_handler(irq_num - 0x20);
    else
        softint_handlers[irq_num - 0x30]();
    __asm__ volatile ("sti\n");
}
