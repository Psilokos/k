#ifndef TIMER_H
# define TIMER_H

# include <k/types.h>

void timer_tick(void);

void timer_init(void);
u32  timer_get_time(void);
void timer_sleep(u32 duration);

#endif
