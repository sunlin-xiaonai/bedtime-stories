#pragma once
#include <stddef.h>
#include <stdint.h>
#include "catalog.h"
#define STORY_FILE_MAX (2 * 1024 * 1024)
size_t story_packet_offset(const uint8_t *data, size_t bytes, size_t packets);
int story_clip_valid(const uint8_t *data, size_t bytes);
int story_valid_id(const char *id);
size_t story_local_scan(const char *dir, const story_entry_t *reserved, size_t reserved_count,
                        story_entry_t *entries, size_t limit);
/* Returns the next allowed story, wrapping; repeat_one keeps the current story.
 * Returns SIZE_MAX when a manually selected track outside the featured catalog ends. */
size_t story_next(size_t current, const unsigned char *allowed, size_t count, int repeat_one);
/* Move one available entry in either direction, wrapping. */
size_t story_step(size_t current, const unsigned char *available, size_t count, int direction);
