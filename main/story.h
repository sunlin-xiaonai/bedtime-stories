#pragma once
#include <stddef.h>
#include <stdint.h>
size_t story_packet_offset(const uint8_t *data, size_t bytes, size_t packets);
int story_valid_id(const char *id);
/* Returns the next allowed story, wrapping; repeat_one keeps the current story. */
size_t story_next(size_t current, const unsigned char *allowed, size_t count, int repeat_one);
/* Move one available entry in either direction, wrapping. */
size_t story_step(size_t current, const unsigned char *available, size_t count, int direction);
