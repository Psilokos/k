#include "timer.h"

static u32 ticks;

void
timer_tick(void)
{
    ++ticks;
}

void
timer_init(void)
{
}

u32
timer_get_time(void)
{
    return ticks; /* FIXME */
}

void
timer_sleep(u32 duration)
{
    u32 const ticks_0 = ticks;
    while (ticks - ticks_0 < duration)
        ;
}
