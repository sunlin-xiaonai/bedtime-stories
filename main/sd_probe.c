#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "driver/sdmmc_host.h"
#include "driver/usb_serial_jtag.h"
#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mbedtls/sha256.h"
#include "sdmmc_cmd.h"
#include "story.h"
#include "catalog.h"
#include "yaykit_app.h"

#define MOUNT "/sdcard"
#define DIR MOUNT "/bedtime"
#define CHUNK 512
#define FILE_MAX (2 * 1024 * 1024)

static SemaphoreHandle_t sd_lock;
static esp_err_t mount_card(sdmmc_card_t **card) {
    if (!sd_lock || xSemaphoreTake(sd_lock, pdMS_TO_TICKS(5000)) != pdTRUE) return ESP_ERR_TIMEOUT;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1; slot.clk = 2; slot.cmd = 1; slot.d0 = 3;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    const esp_vfs_fat_sdmmc_mount_config_t cfg = {.format_if_mount_failed = false, .max_files = 4};
    esp_err_t err = esp_vfs_fat_sdmmc_mount(MOUNT, &host, &slot, &cfg, card);
    if (err != ESP_OK) xSemaphoreGive(sd_lock);
    return err;
}
static esp_err_t unmount_card(sdmmc_card_t *card) {
    esp_err_t err = esp_vfs_fat_sdcard_unmount(MOUNT, card);
    xSemaphoreGive(sd_lock);
    return err;
}
#if CONFIG_YAY_DEV
static int sdprobe(int argc, char **argv) {
    (void)argc; (void)argv;
    sdmmc_card_t *card = NULL;
    esp_err_t err = mount_card(&card);
    if (err != ESP_OK) { printf("BEDTIME_SD error=%s\n", esp_err_to_name(err)); return 1; }
    uint64_t bytes = (uint64_t)card->csd.capacity * card->csd.sector_size;
    printf("BEDTIME_SD ready capacity_mb=%" PRIu64 "\n", bytes / (1024 * 1024));
    err = unmount_card(card);
    printf("BEDTIME_SD unmount=%s\n", esp_err_to_name(err));
    return err != ESP_OK;
}
#endif
static int hex_digest(const char *text, unsigned char out[32]) {
    if (!text || strlen(text) != 64) return 0;
    for (int i = 0; i < 32; i++) {
        unsigned char a = (unsigned char)text[i*2], b = (unsigned char)text[i*2+1];
        if (!isxdigit(a) || !isxdigit(b)) return 0;
        a = (unsigned char)tolower(a); b = (unsigned char)tolower(b);
        out[i] = (unsigned char)(((a <= '9' ? a-'0' : a-'a'+10) << 4) | (b <= '9' ? b-'0' : b-'a'+10));
    }
    return 1;
}
#if CONFIG_YAY_DEV
static int file_hash(const char *path, size_t expected, const unsigned char hash[32]) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    mbedtls_sha256_context sha; mbedtls_sha256_init(&sha); mbedtls_sha256_starts(&sha, 0);
    unsigned char buf[CHUNK], actual[32]; size_t n, total = 0;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) { mbedtls_sha256_update(&sha, buf, n); total += n; }
    int ok = !ferror(f) && total == expected && mbedtls_sha256_finish(&sha, actual) == 0 && !memcmp(actual, hash, 32);
    mbedtls_sha256_free(&sha); fclose(f); return ok;
}
static int sdload(int argc, char **argv) {
    unsigned char expected[32], actual[32], buf[CHUNK]; char *end = NULL;
    if (argc != 4 || !story_valid_id(argv[1]) || !hex_digest(argv[3], expected)) {
        puts("SDLOAD_ERROR invalid arguments"); return 1;
    }
    errno = 0; unsigned long len = strtoul(argv[2], &end, 10);
    if (errno || !end || *end || !len || len > FILE_MAX) { puts("SDLOAD_ERROR invalid length"); return 1; }
    yk_app_input();
    sdmmc_card_t *card = NULL; esp_err_t err = mount_card(&card);
    if (err != ESP_OK) { printf("SDLOAD_ERROR card=%s\n", esp_err_to_name(err)); return 1; }
    int rc = 1, fd = -1; bool created = false;
    char path[48], tmp[48];
    snprintf(path, sizeof(path), DIR "/%s.bin", argv[1]);
    snprintf(tmp, sizeof(tmp), DIR "/%s.tmp", argv[1]);
    if (mkdir(DIR, 0755) != 0 && errno != EEXIST) { puts("SDLOAD_ERROR directory"); goto done; }
    struct stat st;
    if (stat(path, &st) == 0) {
        if (file_hash(path, len, expected)) { printf("SDLOAD_EXISTS %s\n", argv[1]); rc = 0; }
        else puts("SDLOAD_ERROR existing file differs; preserved");
        goto done;
    }
    if (errno != ENOENT) { puts("SDLOAD_ERROR stat"); goto done; }
    fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0) { puts("SDLOAD_ERROR temporary file exists or cannot create"); goto done; }
    created = true;
    mbedtls_sha256_context sha; mbedtls_sha256_init(&sha); mbedtls_sha256_starts(&sha, 0);
    size_t received = 0;
    printf("SDLOAD_READY %d\n", CHUNK); fflush(stdout);
    while (received < len) {
        size_t want = len - received < CHUNK ? len - received : CHUNK, got = 0;
        while (got < want) {
            int n = usb_serial_jtag_read_bytes(buf + got, want - got, pdMS_TO_TICKS(15000));
            if (n <= 0) { puts("SDLOAD_ERROR timeout"); goto receiving_done; }
            got += (size_t)n;
        }
        size_t written = 0;
        while (written < got) {
            ssize_t n = write(fd, buf + written, got - written);
            if (n <= 0) { puts("SDLOAD_ERROR write"); goto receiving_done; }
            written += (size_t)n;
        }
        if (mbedtls_sha256_update(&sha, buf, got) != 0) { puts("SDLOAD_ERROR hash"); goto receiving_done; }
        received += got;
        printf("SDLOAD_ACK %u\n", (unsigned)received); fflush(stdout);
    }
    if (mbedtls_sha256_finish(&sha, actual) != 0 || memcmp(actual, expected, 32)) { puts("SDLOAD_ERROR mismatch"); goto receiving_done; }
    if (fsync(fd) != 0) { puts("SDLOAD_ERROR sync"); goto receiving_done; }
    if (close(fd) != 0) { fd = -1; puts("SDLOAD_ERROR close"); goto receiving_done; }
    fd = -1;
    if (rename(tmp, path) != 0) { puts("SDLOAD_ERROR rename"); goto receiving_done; }
    created = false; rc = 0;
    printf("SDLOAD_OK %s %lu\n", argv[1], len);
receiving_done:
    mbedtls_sha256_free(&sha);
done:
    if (fd >= 0) close(fd);
    if (created) unlink(tmp); // Only removes the temporary file this invocation created.
    err = unmount_card(card);
    if (err != ESP_OK) { printf("SDLOAD_ERROR unmount=%s\n", esp_err_to_name(err)); rc = 1; }
    fflush(stdout); return rc;
}
#endif
esp_err_t sd_scan_stories(unsigned char *available, size_t count) {
    if (!available || count != story_catalog_count) return ESP_ERR_INVALID_ARG;
    sdmmc_card_t *card = NULL;
    esp_err_t err = mount_card(&card);
    if (err != ESP_OK) return err;
    for (size_t i = 0; i < count; i++) {
        char path[48]; struct stat st;
        snprintf(path, sizeof(path), DIR "/%s.bin", story_catalog[i].id);
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size == story_catalog[i].size)
            available[i] = 1;
    }
    return unmount_card(card);
}
uint8_t *sd_read_story(const story_entry_t *book, size_t *bytes) {
    if (!book || !bytes || !story_valid_id(book->id) || !book->size || book->size > FILE_MAX) return NULL;
    unsigned char expected[32], actual[32];
    if (!hex_digest(book->sha256, expected)) return NULL;
    sdmmc_card_t *card = NULL;
    if (mount_card(&card) != ESP_OK) return NULL;
    char path[48]; snprintf(path, sizeof(path), DIR "/%s.bin", book->id);
    struct stat st; FILE *f = NULL; uint8_t *data = NULL;
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size == book->size &&
        (f = fopen(path, "rb")) != NULL &&
        (data = heap_caps_malloc(book->size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) != NULL) {
        const size_t got = fread(data, 1, book->size, f);
        if (got != book->size || ferror(f) ||
            mbedtls_sha256(data, book->size, actual, 0) != 0 || memcmp(actual, expected, 32)) {
            free(data); data = NULL;
        }
    }
    if (f) fclose(f);
    if (unmount_card(card) != ESP_OK) { free(data); data = NULL; }
    if (data) *bytes = book->size;
    return data;
}
void sd_probe_register(void) {
    sd_lock = xSemaphoreCreateMutex();
    assert(sd_lock);
#if CONFIG_YAY_DEV
    const esp_console_cmd_t probe = {.command="sdprobe", .help="Check TF card without formatting or reading user files", .func=sdprobe};
    const esp_console_cmd_t load = {.command="sdload", .help="Import a verified story to the app's own SD folder", .func=sdload};
    ESP_ERROR_CHECK(esp_console_cmd_register(&probe));
    ESP_ERROR_CHECK(esp_console_cmd_register(&load));
#endif
}
