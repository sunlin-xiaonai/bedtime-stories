#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "catalog.h"
#include "cjk_font.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "player.h"
#include "sd_probe.h"
#include "story.h"
#include "yaykit_app.h"
#include "yaykit_key.h"
#include "yaykit_link.h"
#include "yaykit_speak.h"
#include "yaykit_ui.h"
#include "yk_hint.h"

#define APP_SLUG "bedtime-stories"
#define PREFS "yk_bedtime"
#define FRAME_US 20000LL
#define SHELF_PANEL 0x202A23

static unsigned char available[STORY_COUNT], autoplay[STORY_COUNT];
static size_t selected;
static bool playing, library, repeat_one, playback_error;
static unsigned timer_choice;
static const unsigned timer_minutes[] = {15, 30, 60, 0};
static int64_t deadline_us, position_us, last_input_us;
static uint32_t play_generation;
static volatile bool key_tap;
static volatile int test_play = -1;
static volatile int test_timer = -1;
static volatile int test_mode = -1;
static volatile int test_shelf = 0;
static volatile int test_scroll = -1;
static volatile int test_row = -1;
static volatile bool test_inspect, test_redraw;
extern const uint8_t plant_img[] asm("_binary_plant_bin_start");
extern const uint8_t book_img[] asm("_binary_book_bin_start");
extern const uint8_t animal_img[] asm("_binary_animal_bin_start");
extern const uint8_t train_img[] asm("_binary_train_bin_start");
extern const uint8_t cat_img[] asm("_binary_cat_bin_start");
extern const uint8_t plant_thumb[] asm("_binary_plant_thumb_bin_start");
extern const uint8_t book_thumb[] asm("_binary_book_thumb_bin_start");
extern const uint8_t animal_thumb[] asm("_binary_animal_thumb_bin_start");
extern const uint8_t train_thumb[] asm("_binary_train_thumb_bin_start");
extern const uint8_t cat_thumb[] asm("_binary_cat_thumb_bin_start");
static lv_image_dsc_t pictures[5];
static lv_image_dsc_t thumbnails[5];
static lv_obj_t *shelf_rows[STORY_COUNT];
static lv_obj_t *home_view, *list_view, *list_rows, *heading, *name_label, *hint_label, *book_shape;
static lv_obj_t *library_title, *mode_label, *timer_label;

static bool on_key(const yk_key_event_t *ev, void *ctx) {
    (void)ctx;
    if (ev->type != YK_KEY_TAP) return false;
    key_tap = true;
    return true;
}
static void activity(void) { last_input_us = esp_timer_get_time(); yk_app_input(); }
static void save_prefs(void) {
    nvs_handle_t h;
    if (nvs_open(PREFS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_u8(h, "timer", (uint8_t)timer_choice);
    nvs_set_u8(h, "one", repeat_one ? 1 : 0);
    nvs_commit(h);
    nvs_close(h);
}
static void load_prefs(void) {
    nvs_handle_t h;
    uint8_t timer = 0, one = 0;
    if (nvs_open(PREFS, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u8(h, "timer", &timer); nvs_get_u8(h, "one", &one); nvs_close(h);
    }
    timer_choice = timer < sizeof(timer_minutes)/sizeof(timer_minutes[0]) ? timer : 0;
    repeat_one = one == 1;
}
static void set_deadline(int64_t now) {
    unsigned minutes = timer_minutes[timer_choice];
    deadline_us = minutes ? now + (int64_t)minutes * 60 * 1000000 : 0;
}
static void update_controls(void) {
    lv_label_set_text(mode_label, repeat_one ? "单篇循环" : "精选循环");
    char timer[24];
    if (timer_minutes[timer_choice]) snprintf(timer, sizeof(timer), "%u 分钟", timer_minutes[timer_choice]);
    else snprintf(timer, sizeof(timer), "不限时");
    lv_label_set_text(timer_label, timer);
}
static void refresh(void) {
    player_status_t state = player_status();
    const char *title = playback_error ? "故事暂不可用" :
        playing && state.generation == play_generation && state.state == PLAYER_LOADING ? "正在准备" :
        playing ? "正在播放" : "晚安故事";
    lv_label_set_text(heading, title);
    lv_label_set_text(name_label, story_catalog[selected].title);
    lv_label_set_text(hint_label, playback_error ? "上滑选择其他故事" :
                      playing ? "轻点屏幕暂停" :
                      !story_catalog[selected].bedtime_ok ? "家长陪同 · 轻点播放" : "轻点屏幕播放");
    lv_image_set_src(book_shape, &pictures[story_catalog[selected].art]);
    lv_obj_set_hidden(home_view, library);
    lv_obj_set_hidden(list_view, !library);
    for (size_t i=0; i<story_catalog_count; i++)
        if (shelf_rows[i]) lv_obj_set_state(shelf_rows[i], LV_STATE_CHECKED, i == selected);
    update_controls();
}
static void stop_audio(void) { player_stop(); playing = false; }
static void pause_story(void) {
    if (!playing) return;
    player_status_t s = player_status();
    if (s.generation == play_generation && s.state == PLAYER_PLAYING) {
        int64_t elapsed = esp_timer_get_time() - s.started_us;
        // Playback is buffered; replay a little on resume instead of skipping words.
        if (elapsed > 300000) position_us += elapsed - 300000;
    }
    stop_audio();
}
static void start_story(void) {
    playback_error = false;
    playing = true;
    play_generation = player_play(&story_catalog[selected], position_us);
    if (!deadline_us) set_deadline(esp_timer_get_time());
}
static void toggle(void) { if (playing) pause_story(); else start_story(); refresh(); }
static void choose(size_t index, bool play) {
    if (index >= story_catalog_count || !available[index]) return;
    stop_audio(); selected = index; position_us = 0; library = false; playback_error = false;
    if (play) start_story();
    refresh();
}
static void on_screen_click(lv_event_t *e) { (void)e; activity(); if (!library) toggle(); }
static void on_item(lv_event_t *e) { activity(); choose((size_t)(uintptr_t)lv_event_get_user_data(e), true); }
static void on_mode(lv_event_t *e) { (void)e; activity(); repeat_one = !repeat_one; save_prefs(); refresh(); }
static void on_timer(lv_event_t *e) {
    (void)e; activity();
    timer_choice = (timer_choice + 1) % (sizeof(timer_minutes)/sizeof(timer_minutes[0]));
    if (deadline_us || playing) set_deadline(esp_timer_get_time());
    save_prefs(); refresh();
}
static void on_gesture(lv_event_t *e) {
    (void)e; activity();
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (library) {
        // Vertical gestures belong to the scrollable shelf; the side key goes back.
        if (dir == LV_DIR_RIGHT) { library = false; refresh(); }
    } else if (dir == LV_DIR_TOP) { library = true; refresh(); }
    else if (dir == LV_DIR_LEFT) choose(story_step(selected, available, story_catalog_count, 1), false);
    else if (dir == LV_DIR_RIGHT) choose(story_step(selected, available, story_catalog_count, -1), false);
}
static void tick(lv_timer_t *timer) {
    (void)timer;
    const int64_t now = esp_timer_get_time();
    if (key_tap) {
        key_tap = false; activity();
        if (board_display_asleep()) board_display_wake(board_pref_brightness());
        else if (library) { library = false; refresh(); }
        else if (playing) { pause_story(); refresh(); }
        else board_display_sleep();
    }
#if CONFIG_YAY_DEV
    if (test_shelf) {
        int view = test_shelf; test_shelf = 0; activity();
        if (board_display_asleep()) board_display_wake(board_pref_brightness());
        library = view == 1; refresh();
    }
    if (test_scroll >= 0) { int y = test_scroll; test_scroll = -1; activity(); lv_obj_scroll_to_y(list_rows, y, LV_ANIM_OFF); }
    if (test_row >= 0) {
        int rownum = test_row; test_row = -1;
        if (library && (uint32_t)rownum < lv_obj_get_child_count(list_rows))
            lv_obj_send_event(lv_obj_get_child(list_rows, rownum), LV_EVENT_CLICKED, NULL);
    }
    if (test_inspect) {
        test_inspect = false;
        lv_area_t title, mode, time;
        lv_obj_get_coords(library_title, &title);
        lv_obj_get_coords(lv_obj_get_parent(mode_label), &mode);
        lv_obj_get_coords(lv_obj_get_parent(timer_label), &time);
        printf("BEDTEST_UI shelf=%d rows_scroll=%ld view_scroll=%ld title=%ld,%ld mode=%ld,%ld timer=%ld,%ld hidden=%d,%d,%d\n",
               library, (long)lv_obj_get_scroll_y(list_rows), (long)lv_obj_get_scroll_y(list_view),
               (long)title.x1, (long)title.y1, (long)mode.x1, (long)mode.y1, (long)time.x1, (long)time.y1,
               lv_obj_is_hidden(library_title),
               lv_obj_is_hidden(lv_obj_get_parent(mode_label)),
               lv_obj_is_hidden(lv_obj_get_parent(timer_label)));
    }
    if (test_redraw) { test_redraw = false; lv_obj_invalidate(lv_screen_active()); lv_refr_now(NULL); }
    if (test_play >= 0) { int idx = test_play; test_play = -1; choose((size_t)idx, true); }
    if (test_timer >= 0) { int seconds = test_timer; test_timer = -1; deadline_us = seconds ? now + (int64_t)seconds * 1000000 : 0; }
    if (test_mode >= 0) { repeat_one = test_mode != 0; test_mode = -1; refresh(); }
#endif
    int off_s = board_pref_screen_off_s();
    if (off_s > 0 && !board_display_asleep() && now - last_input_us >= (int64_t)off_s * 1000000) board_display_sleep();
    if (deadline_us && now >= deadline_us) {
        stop_audio(); position_us = 0; deadline_us = 0; playback_error = false;
        printf("BEDTIME_TIMER_STOP\n"); refresh(); return;
    }
    if (playing) {
        player_status_t s = player_status();
        if (s.generation == play_generation && s.state == PLAYER_DONE) {
            size_t next = story_next(selected, autoplay, story_catalog_count, repeat_one);
            printf("BEDTIME_NEXT from=%s to=%s mode=%s\n", story_catalog[selected].id, story_catalog[next].id, repeat_one ? "one" : "selected");
            choose(next, true);
        } else if (s.generation == play_generation && s.state == PLAYER_ERROR) {
            stop_audio(); position_us = 0; deadline_us = 0; playback_error = true;
            printf("BEDTIME_PLAY_ERROR id=%s\n", story_catalog[selected].id); refresh();
        } else if (s.generation == play_generation && s.state == PLAYER_PLAYING &&
                   strcmp(lv_label_get_text(heading), "正在播放")) refresh();
    }
}
static void on_leave(void *ctx) { (void)ctx; yk_speak_cancel(); }
static esp_err_t on_command(const char *name, const cJSON *args, cJSON *reply, const char **code, void *ctx) {
    (void)ctx; return yk_app_command(name, args, reply, code);
}
#if CONFIG_YAY_DEV
static int bedtest(int argc, char **argv) {
    if (argc == 1 || !strcmp(argv[1], "status")) {
        player_status_t s = player_status();
        unsigned count = 0, safe = 0;
        for (size_t i=0; i<story_catalog_count; i++) { count += available[i] != 0; safe += autoplay[i] != 0; }
        printf("BEDTEST id=%s playing=%d worker=%d gen=%u deadline=%d one=%d count=%u selected=%u\n", story_catalog[selected].id,
               playing, s.state, (unsigned)s.generation, deadline_us != 0, repeat_one, count, safe);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "shelf")) { test_shelf=1; return 0; }
    if (argc == 2 && !strcmp(argv[1], "home")) { test_shelf=2; return 0; }
    if (argc == 2 && !strcmp(argv[1], "inspect")) { test_inspect=true; return 0; }
    if (argc == 2 && !strcmp(argv[1], "redraw")) { test_redraw=true; return 0; }
    if (argc == 3 && !strcmp(argv[1], "scroll")) { int y=atoi(argv[2]); if (y>=0 && y<=3000) { test_scroll=y; return 0; } }
    if (argc == 3 && !strcmp(argv[1], "row")) { int n=atoi(argv[2]); if (n>=0 && n<STORY_COUNT) { test_row=n; return 0; } }
    if (argc == 3 && !strcmp(argv[1], "play")) {
        for (size_t i=0; i<story_catalog_count; i++) if (!strcmp(argv[2], story_catalog[i].id) && available[i]) { test_play=(int)i; return 0; }
    }
    if (argc == 3 && !strcmp(argv[1], "timer")) { int n=atoi(argv[2]); if (n>=0 && n<=60) { test_timer=n; return 0; } }
    if (argc == 3 && !strcmp(argv[1], "mode")) {
        if (!strcmp(argv[2], "one")) { test_mode=1; return 0; }
        if (!strcmp(argv[2], "all")) { test_mode=0; return 0; }
    }
    puts("BEDTEST_ERROR"); return 1;
}
#endif
static void console_start(void) {
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    cfg.prompt = APP_SLUG ">"; cfg.task_stack_size = 8192;
    esp_console_dev_usb_serial_jtag_config_t usb = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&usb, &cfg, &repl));
    esp_console_register_help_command();
    ESP_ERROR_CHECK(yk_link_register_console()); ESP_ERROR_CHECK(yk_app_register_console());
    yk_ui_register_snapshot(); sd_probe_register();
#if CONFIG_YAY_DEV
    const esp_console_cmd_t test = {.command="bedtest",.help="Exercise story playback",.func=bedtest};
    ESP_ERROR_CHECK(esp_console_cmd_register(&test));
#endif
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
static lv_obj_t *control(lv_obj_t *parent, int x, lv_event_cb_t cb, lv_obj_t **label) {
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_remove_style_all(btn); lv_obj_set_pos(btn, x, 330); lv_obj_set_size(btn, 120, 64);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x304236), 0); lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x526b52), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 20, 0); lv_obj_set_scrollable(btn, false);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    *label = yk_ui_label(btn, "", 0, 20, 120, YK_SIZE_STATUS, YK_INK);
    return btn;
}
static void rebuild_shelf(void) {
    lv_obj_clean(list_rows);
    memset(shelf_rows, 0, sizeof(shelf_rows));
    size_t rownum = 0;
    for (size_t pos=0; pos<story_catalog_count; pos++) {
        const size_t lead = 3, featured = THOMAS_COUNT + CAT_COUNT;
        size_t i = pos < lead ? pos : pos < lead + CAT_COUNT ? CAT_INDEX + pos - lead :
                   pos < lead + featured ? THOMAS_FIRST + pos - lead - CAT_COUNT : pos - featured;
        if (!available[i]) continue;
        lv_obj_t *row = lv_obj_create(list_rows);
        shelf_rows[i] = row;
        lv_obj_remove_style_all(row); lv_obj_set_pos(row, 0, (int)rownum * 75); lv_obj_set_size(row, 316, 69);
        lv_obj_set_style_bg_color(row, lv_color_hex(SHELF_PANEL), 0); lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x405548), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(row, lv_color_hex(YK_SAGE), LV_STATE_CHECKED);
        lv_obj_set_style_border_width(row, 2, LV_STATE_CHECKED);
        lv_obj_set_style_radius(row, 14, 0); lv_obj_set_scrollable(row, false);
        lv_obj_set_event_bubble(row, true); lv_obj_add_event_cb(row, on_item, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_t *cover = lv_image_create(row);
        lv_image_set_src(cover, &thumbnails[story_catalog[i].art]);
        lv_obj_set_pos(cover, 9, 11); lv_obj_set_clickable(cover, false);
        const char *title = story_catalog[i].title;
        const char *category = story_catalog[i].bedtime_ok ? "中文故事" : "建议家长陪同";
        if (i >= THOMAS_FIRST) {
            const char *separator = strstr(title, "·");
            if (separator) title = separator + strlen("·");
            category = i >= CAT_INDEX ? "猫和老鼠 · 英文" : "托马斯 · 英文";
        }
        lv_obj_t *label = yk_ui_label(row, title, 72, 9, 230, YK_SIZE_SUBTITLE_SMALL, YK_INK);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
        lv_obj_t *sub = yk_ui_label(row, category, 72, 40, 230, YK_SIZE_MIN,
                                    story_catalog[i].bedtime_ok ? YK_SAGE_TEXT : YK_CORAL_TEXT);
        lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_LEFT, 0);
        rownum++;
    }
    char count[40]; snprintf(count, sizeof(count), "故事书架 · %u 篇", (unsigned)rownum);
    lv_label_set_text(library_title, count);
}
static void shelf_star(lv_obj_t *parent, int x, int y, int size, uint32_t color) {
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot); lv_obj_set_pos(dot, x, y); lv_obj_set_size(dot, size, size);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), 0); lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_clickable(dot, false); lv_obj_set_scrollable(dot, false);
}
void app_main(void) {
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) ESP_LOGE(APP_SLUG, "NVS unavailable: %s", esp_err_to_name(err));
    ESP_ERROR_CHECK(yk_app_start(&(yk_app_config_t){.slug=APP_SLUG,.version=APP_VERSION}));
    yk_app_on_leave(on_leave, NULL);
    if (!board_display_start_opts(&(board_display_opts_t){.start_dark=true})) return;
    load_prefs();
    ESP_ERROR_CHECK(yk_speak_init()); yk_speak_volume(board_pref_volume());
    ESP_ERROR_CHECK(yk_key_init(&(yk_key_config_t){.delay_tap=true}));
    ESP_ERROR_CHECK(yk_app_on_key(on_key, NULL));
    const yk_link_config_t link = {.board="tikpal-r1",.app_slug=APP_SLUG,.app_version=APP_VERSION,.layout="yaykit-v1",.idle_seconds=90,.on_command=on_command};
    ESP_ERROR_CHECK(yk_link_init(&link));
    yk_launch_t launch; (void)yk_app_launch(&launch);
    for (size_t i=0; i<story_catalog_count && i<STORY_COUNT; i++) available[i] = story_catalog[i].builtin_start != NULL;
    board_lock(-1); ESP_ERROR_CHECK(cjk_font_init());
    lv_obj_t *screen = lv_obj_create(NULL); lv_obj_set_scrollable(screen, false);
    lv_obj_set_style_bg_color(screen, lv_color_hex(YK_NIGHT), 0);
    lv_obj_add_event_cb(screen, on_gesture, LV_EVENT_GESTURE, NULL);
    home_view=lv_obj_create(screen); lv_obj_remove_style_all(home_view); lv_obj_set_size(home_view,466,466); lv_obj_set_scrollable(home_view,false);
    lv_obj_set_event_bubble(home_view, true); lv_obj_add_event_cb(home_view,on_screen_click,LV_EVENT_CLICKED,NULL);
    heading=yk_ui_label(home_view,"",0,65,466,YK_SIZE_STATUS,YK_SAGE_TEXT);
    const uint8_t *art[] = {plant_img, book_img, animal_img, train_img, cat_img};
    for (size_t i=0; i<sizeof(art)/sizeof(art[0]); i++) pictures[i] = (lv_image_dsc_t){.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565,.w=160,.h=140,.stride=320},.data_size=160*140*2,.data=art[i]};
    const uint8_t *small[] = {plant_thumb, book_thumb, animal_thumb, train_thumb, cat_thumb};
    for (size_t i=0; i<sizeof(small)/sizeof(small[0]); i++) thumbnails[i] = (lv_image_dsc_t){.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565,.w=52,.h=46,.stride=104},.data_size=52*46*2,.data=small[i]};
    lv_obj_t *halo=lv_obj_create(home_view); lv_obj_remove_style_all(halo);
    lv_obj_set_pos(halo, 143, 96); lv_obj_set_size(halo, 180, 180);
    lv_obj_set_style_radius(halo, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(halo, lv_color_hex(0x45584a), 0);
    lv_obj_set_style_border_width(halo, 1, 0);
    lv_obj_set_clickable(halo, false); lv_obj_set_scrollable(halo, false);
    shelf_star(home_view, 132, 145, 5, YK_AMBER); shelf_star(home_view, 331, 160, 4, YK_SAGE);
    shelf_star(home_view, 141, 238, 3, YK_SAGE_TEXT); shelf_star(home_view, 322, 238, 5, YK_AMBER);
    book_shape=lv_image_create(home_view); lv_obj_set_pos(book_shape,153,110); lv_obj_set_clickable(book_shape,false);
    name_label=yk_ui_label(home_view,"",55,282,356,YK_SIZE_HEADING,YK_INK);
    lv_label_set_long_mode(name_label, LV_LABEL_LONG_DOT);
    hint_label=yk_ui_label(home_view,"",0,325,466,YK_SIZE_STATUS,YK_CREAM);
    yk_ui_label(home_view,"左右滑动 · 上滑书架",0,378,466,YK_SIZE_MIN,YK_INK2);
    list_view=lv_obj_create(screen); lv_obj_remove_style_all(list_view); lv_obj_set_size(list_view,466,466); lv_obj_set_scrollable(list_view,false); lv_obj_set_event_bubble(list_view,true);
    library_title=yk_ui_label(list_view,"",0,55,466,YK_SIZE_STATUS,YK_SAGE_TEXT);
    shelf_star(list_view, 108, 65, 5, YK_AMBER); shelf_star(list_view, 353, 66, 4, YK_SAGE);
    list_rows=lv_obj_create(list_view); lv_obj_remove_style_all(list_rows); lv_obj_set_pos(list_rows,75,100); lv_obj_set_size(list_rows,316,220);
    lv_obj_set_scrollable(list_rows,true); lv_obj_set_scroll_dir(list_rows,LV_DIR_VER); lv_obj_set_scrollbar_mode(list_rows,LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_gesture_bubble(list_rows, true);
    control(list_view, 104, on_mode, &mode_label); control(list_view, 242, on_timer, &timer_label);
    yk_ui_label(list_view,"上下滑动 · 侧键返回",0,401,466,YK_SIZE_MIN,YK_INK2);
    rebuild_shelf(); refresh(); lv_screen_load(screen); lv_refr_now(NULL); board_brightness_set(board_pref_brightness());
    if (yk_hint_should(YK_HINT_APPS)) yk_hint_show(screen,YK_HINT_APPS,NULL);
    last_input_us = esp_timer_get_time(); lv_timer_create(tick,100,NULL); board_unlock();
    console_start(); ESP_ERROR_CHECK(player_init()); yk_app_ready();
    unsigned char found[STORY_COUNT] = {0};
    for (size_t i=0; i<story_catalog_count; i++) found[i] = story_catalog[i].builtin_start != NULL;
    err = sd_scan_stories(found, STORY_COUNT);
    if (err != ESP_OK) ESP_LOGW(APP_SLUG, "SD card unavailable: %s", esp_err_to_name(err));
    board_lock(-1);
    for (size_t i=0; i<story_catalog_count; i++) {
        available[i] = found[i]; autoplay[i] = found[i] && story_catalog[i].bedtime_ok;
    }
    rebuild_shelf(); refresh(); board_unlock();
    printf("BEDTIME_CATALOG count=%u card=%s\n", (unsigned)story_catalog_count, err == ESP_OK ? "ready" : "unavailable");
}
