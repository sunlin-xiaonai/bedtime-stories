#include <stdio.h>
#include <string.h>
#include "board.h"
#include "cjk_font.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "story.h"
#include "yaykit_app.h"
#include "yaykit_key.h"
#include "yaykit_link.h"
#include "yaykit_speak.h"
#include "yaykit_ui.h"
#include "yk_hint.h"

#define APP_SLUG "bedtime-stories"
#define SLEEP_US (15LL * 60 * 1000000)
#define FRAME_US 20000LL

extern const uint8_t khalai_start[] asm("_binary_khalai_bin_start");
extern const uint8_t khalai_end[] asm("_binary_khalai_bin_end");
extern const uint8_t read_start[] asm("_binary_read_bin_start");
extern const uint8_t read_end[] asm("_binary_read_bin_end");
extern const uint8_t counting_start[] asm("_binary_counting_bin_start");
extern const uint8_t counting_end[] asm("_binary_counting_bin_end");
typedef struct { const char *zh, *en, *mark; const uint8_t *start, *end; uint32_t color; } book_t;
static const book_t books[] = {
    {"和植物聊天的卡莱", "Khalai and plants", "叶", khalai_start, khalai_end, YK_SAGE},
    {"我喜欢看书", "I like to read", "书", read_start, read_end, YK_CORAL},
    {"数数动物", "Counting animals", "星", counting_start, counting_end, YK_AMBER},
};
#define BOOKS (sizeof(books) / sizeof(books[0]))
static size_t selected;
static bool english, playing, library;
static int64_t started_us, deadline_us, position_us, last_input_us;
static volatile bool key_tap;
static lv_obj_t *home_view, *list_view, *heading, *name_label, *mark_label, *hint_label, *book_shape;
static const char *T(const char *zh, const char *en) { return english ? en : zh; }

static bool on_key(const yk_key_event_t *ev, void *ctx) {
    (void)ctx;
    if (ev->type == YK_KEY_TAP) { key_tap = true; return true; }
    return false;
}
static void stop_audio(void) { yk_speak_cancel(); playing = false; }
static void refresh(void) {
    lv_label_set_text(heading, playing ? T("正在播放", "Playing") : T("晚安故事", "Bedtime stories"));
    lv_label_set_text(name_label, english ? books[selected].en : books[selected].zh);
    lv_label_set_text(mark_label, english ? "*" : books[selected].mark);
    lv_label_set_text(hint_label, playing ? T("轻点暂停 · 上滑书架", "Tap to pause · Swipe up") : T("轻点播放 · 上滑书架", "Tap to play · Swipe up"));
    lv_obj_set_style_bg_color(book_shape, lv_color_hex(books[selected].color), 0);
    if (library) { lv_obj_add_flag(home_view, LV_OBJ_FLAG_HIDDEN); lv_obj_remove_flag(list_view, LV_OBJ_FLAG_HIDDEN); }
    else { lv_obj_add_flag(list_view, LV_OBJ_FLAG_HIDDEN); lv_obj_remove_flag(home_view, LV_OBJ_FLAG_HIDDEN); }
}
static void pause_story(void) {
    if (!playing) return;
    int64_t elapsed = esp_timer_get_time() - started_us;
    // Approximate audible position: clip output is buffered, so replay a little on resume.
    if (elapsed > 300000) position_us += elapsed - 300000;
    stop_audio();
}
static void start_story(void) {
    const book_t *b = &books[selected];
    size_t total = (size_t)(b->end - b->start);
    size_t offset = story_packet_offset(b->start, total, (size_t)(position_us / FRAME_US));
    if (offset >= total) { position_us = 0; offset = 0; }
    yk_clip_t clip = {.format = YK_CLIP_OPUS, .data = b->start + offset, .bytes = total - offset};
    if (yk_speak_clip(&clip) == ESP_OK) {
        playing = true; started_us = esp_timer_get_time();
        if (!deadline_us) deadline_us = started_us + SLEEP_US;
    }
}
static void toggle(void) {
    if (playing) pause_story(); else start_story();
    refresh();
}
static void choose(size_t index, bool play) {
    stop_audio(); selected = index; position_us = 0; library = false;
    if (play) start_story();
    refresh();
}
static void activity(void) { last_input_us = esp_timer_get_time(); yk_app_input(); }
static void on_screen_click(lv_event_t *e) { (void)e; activity(); if (!library) toggle(); }
static void on_item(lv_event_t *e) { activity(); choose((size_t)(uintptr_t)lv_event_get_user_data(e), true); }
static void on_gesture(lv_event_t *e) {
    (void)e; activity();
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (library) {
        if (dir == LV_DIR_BOTTOM) { library = false; refresh(); }
    } else if (dir == LV_DIR_TOP) { library = true; refresh(); }
    else if (dir == LV_DIR_LEFT) choose((selected + 1) % BOOKS, false);
    else if (dir == LV_DIR_RIGHT) choose((selected + BOOKS - 1) % BOOKS, false);
}
static void tick(lv_timer_t *timer) {
    (void)timer;
    int64_t now = esp_timer_get_time();
    if (key_tap) {
        key_tap = false; activity();
        if (board_display_asleep()) board_display_wake(yk_app_brightness());
        else if (library) { library = false; refresh(); }
        else if (playing) { pause_story(); refresh(); }
        else board_display_sleep();
    }
    int off_s = yk_app_screen_off_s();
    if (off_s > 0 && !board_display_asleep() && now - last_input_us >= (int64_t)off_s * 1000000) board_display_sleep();
    if (deadline_us && now >= deadline_us) { stop_audio(); position_us = 0; deadline_us = 0; refresh(); }
    if (playing && now - started_us > 1000000 && !yk_speak_busy()) { playing = false; position_us = 0; deadline_us = 0; refresh(); }
}
static void on_leave(void *ctx) { (void)ctx; yk_speak_cancel(); }
static esp_err_t on_command(const char *name, const cJSON *args, cJSON *reply, const char **code, void *ctx) {
    (void)ctx;
    return yk_app_command(name, args, reply, code);
}
static void console_start(void) {
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    cfg.prompt = APP_SLUG ">"; cfg.task_stack_size = 8192;
    esp_console_dev_usb_serial_jtag_config_t usb = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&usb, &cfg, &repl));
    esp_console_register_help_command();
    ESP_ERROR_CHECK(yk_link_register_console()); ESP_ERROR_CHECK(yk_app_register_console());
    yk_ui_register_snapshot(); ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
void app_main(void) {
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) ESP_LOGE(APP_SLUG, "NVS unavailable: %s", esp_err_to_name(err));
    ESP_ERROR_CHECK(yk_app_start(&(yk_app_config_t){.slug=APP_SLUG,.version=APP_VERSION}));
    yk_app_on_leave(on_leave, NULL); english = yk_app_english();
    if (!board_display_start_opts(&(board_display_opts_t){.start_dark=true})) return;
    ESP_ERROR_CHECK(yk_speak_init()); yk_speak_volume(yk_app_volume());
    ESP_ERROR_CHECK(yk_key_init(&(yk_key_config_t){.delay_tap=true}));
    ESP_ERROR_CHECK(yk_app_on_key(on_key, NULL));
    const yk_link_config_t link = {.board="tikpal-r1",.app_slug=APP_SLUG,.app_version=APP_VERSION,.layout="yaykit-v1",.idle_seconds=90,.on_command=on_command};
    ESP_ERROR_CHECK(yk_link_init(&link));
    yk_launch_t launch; (void)yk_app_launch(&launch);
    board_lock(-1); ESP_ERROR_CHECK(cjk_font_init());
    lv_obj_t *screen = lv_obj_create(NULL); lv_obj_set_scrollable(screen, false);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    lv_obj_add_event_cb(screen,on_gesture,LV_EVENT_GESTURE,NULL);
    home_view=lv_obj_create(screen); lv_obj_remove_style_all(home_view); lv_obj_set_size(home_view,466,466); lv_obj_set_scrollable(home_view,false);
    lv_obj_add_flag(home_view, LV_OBJ_FLAG_EVENT_BUBBLE); lv_obj_add_event_cb(home_view,on_screen_click,LV_EVENT_CLICKED,NULL);
    heading=yk_ui_label(home_view,"",0,65,466,YK_SIZE_STATUS,YK_SAGE_TEXT);
    book_shape=lv_obj_create(home_view); lv_obj_remove_style_all(book_shape); lv_obj_set_size(book_shape,124,120); lv_obj_set_pos(book_shape,171,130); lv_obj_set_style_radius(book_shape,14,0); lv_obj_remove_flag(book_shape,LV_OBJ_FLAG_CLICKABLE);
    mark_label=yk_ui_label(home_view,"",0,157,466,YK_SIZE_HEADING,YK_PAPER);
    name_label=yk_ui_label(home_view,"",55,270,356,YK_SIZE_HEADING,YK_INK);
    hint_label=yk_ui_label(home_view,"",0,350,466,YK_SIZE_MIN,YK_INK2);
    list_view=lv_obj_create(screen); lv_obj_remove_style_all(list_view); lv_obj_set_size(list_view,466,466); lv_obj_set_scrollable(list_view,false); lv_obj_add_flag(list_view,LV_OBJ_FLAG_EVENT_BUBBLE);
    yk_ui_label(list_view,T("全部故事","All stories"),0,65,466,YK_SIZE_HEADING,YK_SAGE_TEXT);
    for (size_t i=0;i<BOOKS;i++) {
        lv_obj_t *row=lv_obj_create(list_view); lv_obj_remove_style_all(row); lv_obj_set_pos(row,63,125+(int)i*74); lv_obj_set_size(row,340,66); lv_obj_set_style_bg_color(row,lv_color_hex(YK_PANEL),0); lv_obj_set_style_bg_opa(row,LV_OPA_COVER,0); lv_obj_set_style_radius(row,16,0); lv_obj_set_scrollable(row,false); lv_obj_add_flag(row,LV_OBJ_FLAG_EVENT_BUBBLE); lv_obj_add_event_cb(row,on_item,LV_EVENT_CLICKED,(void *)(uintptr_t)i);
        yk_ui_label(row,english?books[i].en:books[i].zh,0,18,340,YK_SIZE_SUBTITLE_SMALL,YK_INK);
    }
    yk_ui_label(list_view,T("点故事播放 · 下滑返回","Tap a story · Swipe down"),0,370,466,YK_SIZE_MIN,YK_INK2);
    refresh(); lv_screen_load(screen); lv_refr_now(NULL); board_brightness_set(yk_app_brightness());
    if (yk_hint_should(YK_HINT_APPS)) yk_hint_show(screen,YK_HINT_APPS,NULL);
    last_input_us = esp_timer_get_time(); lv_timer_create(tick,100,NULL); board_unlock();
    console_start(); yk_app_ready();
}
