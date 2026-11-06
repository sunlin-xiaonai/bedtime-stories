#include "sleep_timer.h"

void sleep_timer_set(sleep_timer_t *timer, uint32_t minutes) {
    timer->remaining_seconds = (uint64_t)minutes * 60;
    timer->enabled = minutes != 0;
}

bool sleep_timer_advance(sleep_timer_t *timer, uint64_t elapsed_seconds) {
    if (!timer->enabled) return false;
    if (elapsed_seconds < timer->remaining_seconds) {
        timer->remaining_seconds -= elapsed_seconds;
        return false;
    }
    timer->remaining_seconds = 0;
    timer->enabled = false;
    return true;
}
