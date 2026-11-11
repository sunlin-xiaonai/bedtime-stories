#include "story.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
int main(int argc, char **argv) {
    assert(argc == 2);
    const uint8_t data[]={2,0,9,8,1,0,7,0,0};
    assert(story_packet_offset(data,sizeof(data),0)==0);
    assert(story_packet_offset(data,sizeof(data),1)==4);
    assert(story_packet_offset(data,sizeof(data),2)==7);
    assert(story_packet_offset(data,sizeof(data),3)==sizeof(data));
    assert(story_packet_offset(data,3,1)==3);
    assert(story_clip_valid(data, 7));
    assert(!story_clip_valid(data, sizeof(data)));
    assert(!story_clip_valid(data, 3));
    assert(!story_clip_valid(data, 0));
    const uint8_t too_large[1280] = {0xfe, 0x04};
    assert(!story_clip_valid(too_large, sizeof(too_large)));
    assert(story_valid_id("0001"));
    assert(story_valid_id("9999"));
    assert(!story_valid_id("../x"));
    assert(!story_valid_id("01234"));
    assert(!story_valid_id("1"));
    assert(!story_valid_id("12a4"));
    const unsigned char allowed[] = {1, 0, 1};
    assert(story_next(0, allowed, 3, 0) == 2);
    assert(story_next(2, allowed, 3, 0) == 0);
    assert(story_next(1, allowed, 3, 1) == 1);
    const unsigned char none[] = {0, 0};
    assert(story_next(1, none, 2, 0) == 1);
    assert(story_next(0, NULL, 0, 0) == SIZE_MAX);
    const unsigned char available[] = {1, 0, 1};
    assert(story_step(0, available, 3, 1) == 2);
    assert(story_step(0, available, 3, -1) == 2);
    assert(story_step(2, available, 3, 1) == 0);
    assert(story_step(0, NULL, 0, 1) == 0);

    const story_entry_t reserved[] = {{.id = "0089"}};
    story_entry_t local[2];
    assert(story_local_scan(argv[1], reserved, 1, local, 2) == 2);
    assert(!strcmp(local[0].id, "1203"));
    assert(!strcmp(local[1].id, "4826"));
    assert(local[0].size == 3 && local[1].size == 3);
    assert(!local[0].title && !local[0].sha256 && !local[0].bedtime_ok && !local[0].builtin_start);
    assert(story_local_scan(argv[1], reserved, 1, local, 1) == 1);
    assert(!strcmp(local[0].id, "1203"));
    assert(story_local_scan("", reserved, 1, local, 2) == 0);
    assert(story_next(3, allowed, 3, 0) == SIZE_MAX);
    assert(story_next(3, allowed, 3, 1) == 3);
    return 0;
}
