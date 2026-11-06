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
    return 0;
}
