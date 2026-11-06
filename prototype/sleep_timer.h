#ifndef SLEEP_TIMER_H
#define SLEEP_TIMER_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint64_t remaining_seconds;
    bool enabled;
} sleep_timer_t;

/* Zero disables the timer. Call advance with elapsed monotonic seconds. */
void sleep_timer_set(sleep_timer_t *timer, uint32_t minutes);
/* Returns true exactly once when the timer expires. */
bool sleep_timer_advance(sleep_timer_t *timer, uint64_t elapsed_seconds);
#endif
