#include "story.h"
#include <assert.h>
#include <stdint.h>
int main(void) {
    const uint8_t data[]={2,0,9,8,1,0,7,0,0};
    assert(story_packet_offset(data,sizeof(data),0)==0);
    assert(story_packet_offset(data,sizeof(data),1)==4);
    assert(story_packet_offset(data,sizeof(data),2)==7);
    assert(story_packet_offset(data,sizeof(data),3)==sizeof(data));
    assert(story_packet_offset(data,3,1)==3);
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
    assert(story_next(0, NULL, 0, 0) == 0);
    const unsigned char available[] = {1, 0, 1};
    assert(story_step(0, available, 3, 1) == 2);
    assert(story_step(0, available, 3, -1) == 2);
    assert(story_step(2, available, 3, 1) == 0);
    assert(story_step(0, NULL, 0, 1) == 0);
    return 0;
}
