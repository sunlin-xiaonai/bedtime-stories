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
