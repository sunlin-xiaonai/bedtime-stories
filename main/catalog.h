#pragma once
#include <stddef.h>
#include <stdint.h>
#define STORY_COUNT 29
#define LOCAL_STORY_LIMIT 16
typedef struct {
    char id[5];
    const char *title, *sha256;
    uint32_t size;
    uint8_t art, bedtime_ok;
    const uint8_t *builtin_start, *builtin_end;
} story_entry_t;
extern const story_entry_t story_catalog[];
