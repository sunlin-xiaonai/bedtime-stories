#include "story.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

static size_t packet_span(const uint8_t *data, size_t bytes) {
    if (bytes < 2) return 0;
    const size_t len = data[0] | ((size_t)data[1] << 8);
    return len && len <= 1276 && len <= bytes - 2 ? len + 2 : 0;
}
size_t story_packet_offset(const uint8_t *data, size_t bytes, size_t packets) {
    size_t p = 0;
    while (packets--) {
        const size_t span = packet_span(data + p, bytes - p);
        if (!span) return bytes;
        p += span;
    }
    return p;
}
int story_clip_valid(const uint8_t *data, size_t bytes) {
    if (!bytes) return 0;
    while (bytes) {
        const size_t span = packet_span(data, bytes);
        if (!span) return 0;
        data += span;
        bytes -= span;
    }
    return 1;
}
int story_valid_id(const char *id) {
    if (!id) return 0;
    for (size_t i = 0; i < 4; i++) if (id[i] < '0' || id[i] > '9') return 0;
    return id[4] == '\0';
}
size_t story_local_scan(const char *dir, const story_entry_t *reserved, size_t reserved_count,
                        story_entry_t *entries, size_t limit) {
    DIR *files = opendir(dir);
    if (!files) return 0;
    size_t count = 0;
    for (struct dirent *file; (file = readdir(files)) != NULL;) {
        const char *name = file->d_name;
        if (strlen(name) != 8 || strcasecmp(name + 4, ".bin") != 0) continue;
        story_entry_t entry = {.art = 1}; // Generic book cover.
        memcpy(entry.id, name, 4);
        if (!story_valid_id(entry.id)) continue;
        size_t r = 0;
        while (r < reserved_count && strcmp(reserved[r].id, entry.id) != 0) r++;
        if (r != reserved_count) continue;

        size_t pos = 0;
        while (pos < count && strcmp(entries[pos].id, entry.id) < 0) pos++;
        if (pos >= limit || (pos < count && strcmp(entries[pos].id, entry.id) == 0)) continue;
        char path[128];
        const int n = snprintf(path, sizeof(path), "%s/%s", dir, name);
        struct stat st;
        if (n < 0 || (size_t)n >= sizeof(path) || stat(path, &st) != 0 ||
            !S_ISREG(st.st_mode) || st.st_size < 3 || st.st_size > STORY_FILE_MAX) continue;
        entry.size = (uint32_t)st.st_size;
        if (count < limit) count++;
        memmove(&entries[pos + 1], &entries[pos], (count - pos - 1) * sizeof(entry));
        entries[pos] = entry;
    }
    closedir(files);
    return count;
}
size_t story_next(size_t current, const unsigned char *allowed, size_t count, int repeat_one) {
    if (repeat_one) return current;
    if (current >= count) return SIZE_MAX;
    if (!allowed) return current;
    for (size_t i = 1; i <= count; i++) {
        size_t next = (current + i) % count;
        if (allowed[next]) return next;
    }
    return current;
}
size_t story_step(size_t current, const unsigned char *available, size_t count, int direction) {
    if (!count) return 0;
    current %= count;
    if (!available || !direction) return current;
    size_t next = current;
    for (size_t i = 0; i < count; i++) {
        next = direction > 0 ? (next + 1) % count : (next + count - 1) % count;
        if (available[next]) return next;
    }
    return current;
}
