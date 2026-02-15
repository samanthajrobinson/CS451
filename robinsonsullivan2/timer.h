#ifndef TIMER_H
#define TIMER_H

// Start a 1-second repeating timer that calls on_tick() each tick.
void timer_start(void (*on_tick)(void));

#endif

