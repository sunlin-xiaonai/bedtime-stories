#pragma once
#include <stddef.h>
#include <stdint.h>
#include "catalog.h"
#include "esp_err.h"
void sd_probe_register(void);
esp_err_t sd_scan_stories(unsigned char *available, size_t count);
uint8_t *sd_read_story(const story_entry_t *book, size_t *bytes);
