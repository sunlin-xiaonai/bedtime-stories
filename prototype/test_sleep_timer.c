#include "sleep_timer.h"
#include <assert.h>
#include <stdint.h>

int main(void) {
    sleep_timer_t timer = {0};
    assert(!sleep_timer_advance(&timer, 100));
    sleep_timer_set(&timer, 5);
    assert(!sleep_timer_advance(&timer, 299));
    assert(timer.remaining_seconds == 1);
    assert(sleep_timer_advance(&timer, 1));
    assert(!sleep_timer_advance(&timer, 1));
    sleep_timer_set(&timer, 1);
    assert(sleep_timer_advance(&timer, 1000));
    sleep_timer_set(&timer, 1);
    sleep_timer_set(&timer, 0);
    assert(!sleep_timer_advance(&timer, UINT64_MAX));
    sleep_timer_set(&timer, UINT32_MAX);
    assert(timer.remaining_seconds == (uint64_t)UINT32_MAX * 60);
    return 0;
}
