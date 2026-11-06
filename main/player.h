#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "catalog.h"
#include "esp_err.h"

typedef enum { PLAYER_IDLE, PLAYER_LOADING, PLAYER_PLAYING, PLAYER_DONE, PLAYER_ERROR } player_state_t;
typedef struct { uint32_t generation; player_state_t state; int64_t started_us; } player_status_t;
esp_err_t player_init(void);
uint32_t player_play(const story_entry_t *book, int64_t position_us);
void player_stop(void);
player_status_t player_status(void);
