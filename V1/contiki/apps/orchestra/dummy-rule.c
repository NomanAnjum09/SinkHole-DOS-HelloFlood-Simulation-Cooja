//
// Created by Hieun Nguyen on 5/24/2018.
//
#include <net/linkaddr.h>
#include "contiki.h"
#include "orchestra.h"
#include "net/packetbuf.h"

static uint16_t slotframe_handle = 0;
static uint16_t channel_offset = 0;
static struct tsch_slotframe *sf_dummy;

#define DUMMY_HASH(addr) (((addr % 2) == 0) ? 0 : 1)
#define ORCHESTRA_COMMON_SHARED_TYPE              LINK_TYPE_NORMAL

static uint16_t
get_node_timeslot(const linkaddr_t *addr)
{
#if ORCHESTRA_EBSF_PERIOD > 0
    return DUMMY_HASH(addr->u8[LINKADDR_SIZE - 1]);
#else
    return 0xffff;
#endif

}

static int
select_packet(uint16_t *slotframe, uint16_t *timeslot)
{
    /*Select anything*/
    if(slotframe != NULL) {
        *slotframe = slotframe_handle;
    }
    if(timeslot != NULL) {
        *timeslot = 0;
    }
    return 1;

}

static void
init(uint16_t sf_handle)
{
    slotframe_handle = sf_handle;
    channel_offset = slotframe_handle;
    sf_dummy = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_COMMON_SHARED_PERIOD);
    tsch_schedule_add_link(sf_dummy,
                           LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
                           ORCHESTRA_COMMON_SHARED_TYPE,
                           &tsch_broadcast_address,
                           get_node_timeslot(&linkaddr_node_addr),
                           channel_offset);

}

struct orchestra_rule dummy_rule = {
        init,
        NULL,
        select_packet,
        NULL,
        NULL,
};