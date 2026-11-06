#include "story.h"
size_t story_packet_offset(const uint8_t *data, size_t bytes, size_t packets) {
    size_t p = 0;
    while (packets--) {
        if (bytes - p < 2) return bytes;
        size_t n = data[p] | ((size_t)data[p + 1] << 8);
        if (!n || n > bytes - p - 2) return bytes;
        p += n + 2;
    }
    return p;
}
int story_valid_id(const char *id) {
    if (!id) return 0;
    for (size_t i = 0; i < 4; i++) if (id[i] < '0' || id[i] > '9') return 0;
    return id[4] == '\0';
}
size_t story_next(size_t current, const unsigned char *allowed, size_t count, int repeat_one) {
    if (!count) return 0;
    current %= count;
    if (repeat_one || !allowed) return current;
    for (size_t i = 1; i <= count; i++) {
        size_t next = (current + i) % count;
        if (allowed[next]) return next;
    }
    return current;
}
size_t story_step(size_t current, const unsigned char *available, size_t count, int direction) {
    if (!count) return 0;
    current %= count;
    if (!available || !direction) return current;
    size_t next = current;
    for (size_t i = 0; i < count; i++) {
        next = direction > 0 ? (next + 1) % count : (next + count - 1) % count;
        if (available[next]) return next;
    }
    return current;
}
