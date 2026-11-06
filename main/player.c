#include <stdlib.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "player.h"
#include "sd_probe.h"
#include "story.h"
#include "yaykit_speak.h"

#define FRAME_US 20000LL

typedef struct { uint32_t generation; const story_entry_t *book; int64_t position_us; } request_t;
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static request_t wanted;
static player_status_t status;
static TaskHandle_t task;

static request_t request(void) {
    request_t r; portENTER_CRITICAL(&lock); r = wanted; portEXIT_CRITICAL(&lock); return r;
}
static void set_status(uint32_t gen, player_state_t state, int64_t started) {
    portENTER_CRITICAL(&lock);
    if (wanted.generation == gen) status = (player_status_t){.generation=gen, .state=state, .started_us=started};
    portEXIT_CRITICAL(&lock);
}
player_status_t player_status(void) {
    player_status_t s; portENTER_CRITICAL(&lock); s = status; portEXIT_CRITICAL(&lock); return s;
}
uint32_t player_play(const story_entry_t *book, int64_t position_us) {
    portENTER_CRITICAL(&lock);
    wanted.generation++; wanted.book = book; wanted.position_us = position_us;
    status = (player_status_t){.generation=wanted.generation, .state=PLAYER_LOADING};
    const uint32_t gen = wanted.generation;
    portEXIT_CRITICAL(&lock);
    if (task) xTaskNotifyGive(task);
    return gen;
}
void player_stop(void) {
    portENTER_CRITICAL(&lock);
    wanted.generation++; wanted.book = NULL; wanted.position_us = 0;
    status = (player_status_t){.generation=wanted.generation, .state=PLAYER_IDLE};
    portEXIT_CRITICAL(&lock);
    if (task) xTaskNotifyGive(task);
}
static void player_task(void *arg) {
    (void)arg;
    uint32_t active = 0; uint8_t *owned = NULL; bool running = false;
    for (;;) {
        request_t r = request();
        if (r.generation != active) {
            if (running) {
                yk_speak_stop();
                // The speaker reads the clip asynchronously; never release it while busy.
                while (yk_speak_busy()) vTaskDelay(pdMS_TO_TICKS(10));
            }
            running = false; free(owned); owned = NULL; active = r.generation;
            if (r.book) {
                const uint8_t *data = r.book->builtin_start;
                size_t size = data ? (size_t)(r.book->builtin_end - data) : 0;
                if (!data) { owned = sd_read_story(r.book, &size); data = owned; }
                if (request().generation != active) { free(owned); owned = NULL; continue; }
                if (!data) { set_status(active, PLAYER_ERROR, 0); }
                else {
                    size_t offset = story_packet_offset(data, size, (size_t)(r.position_us / FRAME_US));
                    if (offset >= size) offset = 0;
                    yk_clip_t clip = {.format=YK_CLIP_OPUS, .data=data+offset, .bytes=size-offset};
                    if (yk_speak_clip(&clip) == ESP_OK) {
                        int64_t started = esp_timer_get_time(); running = true;
                        set_status(active, PLAYER_PLAYING, started);
                    } else set_status(active, PLAYER_ERROR, 0);
                }
            }
        }
        if (running) {
            player_status_t s = player_status();
            if (s.generation == active && s.state == PLAYER_PLAYING &&
                esp_timer_get_time() - s.started_us > 1000000 && !yk_speak_busy()) {
                running = false; free(owned); owned = NULL;
                set_status(active, PLAYER_DONE, s.started_us);
            }
        }
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));
    }
}
esp_err_t player_init(void) {
    return xTaskCreate(player_task, "story_player", 8192, NULL, 5, &task) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
