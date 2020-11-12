//
// Created by Hieu Nguyen on 5/7/2018.
//
#include "contiki.h"
#include "../../../cpu/msp430/msp430def.h"
#include "sys/node-id.h"
#include "lib/random.h"
#include "net/rpl/rpl-private.h"
#include "deployment.h"
#include "deployment-log.h"
#include "net/ip/simple-udp.h"
#include "stdio.h"

#define SEND_INTERVAL   (30*CLOCK_SECOND)
#define UDP_PORT 1234
#define ROOT_ID 1

static struct simple_udp_connection unicast_connection;

static void receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
    LOGA((void*)data, "App: received");
}

void set_linkaddr_from_id(linkaddr_t *lladdr, uint16_t id){
    /*
     * 00:01.246	ID:1	Rime started with address 193.12.0.0.0.0.0.1
     * fe80::c30c:0:0:1
     * indria
     * 1, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xb3,0xae}}},
     * fe80::212:7401:1:101
     * */
    lladdr->u8[0] = 0xc3;
    lladdr->u8[1] = 0x0c;
    lladdr->u8[2] = 0x00;
    lladdr->u8[3] = 0x00;
    lladdr->u8[4] = 0x00;
    lladdr->u8[5] = 0x00;
    lladdr->u8[6] = 0x00;
    lladdr->u8[7] = id;
}

void set_ipaddr_from_id(uip_ipaddr_t *ipaddr, uint16_t id)
{
    linkaddr_t lladdr;
    static uip_ipaddr_t prefix;
    //
    memcpy(ipaddr, &prefix, 8);
    set_linkaddr_from_id(&lladdr, id);
    uip_ds6_set_addr_iid(ipaddr, (uip_lladdr_t *)&lladdr);
}

int can_send_to(uip_ipaddr_t *ipaddr) {
    return uip_ds6_is_addr_onlink(ipaddr)
           || uip_ds6_route_lookup(ipaddr)
           || uip_ds6_defrt_choose();
}


int app_send_to(uint16_t id, uint32_t seqno, unsigned int to_send_cnt){
    struct app_data data;
    uip_ipaddr_t dest_ipaddr;

    data.magic = UIP_HTONL(LOG_MAGIC);
    data.seqno = UIP_HTONL(seqno);
    data.src = UIP_HTONS(node_id);
    data.dest = UIP_HTONS(id);
    data.hop = 0;

    //set_ipaddr_from_id(&dest_ipaddr, id);
    /*dest_ipadd <- (node_id == 1)*/
    uip_ip6addr(&dest_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0xc30c, 0, 0, 1);
    if(can_send_to(&dest_ipaddr)) {
        LOGA(&data, "App: sending");
        simple_udp_sendto(&unicast_connection, &data, sizeof(data), &dest_ipaddr);
        return 1;
    } else {
        data.seqno = UIP_HTONL(seqno + to_send_cnt - 1);
        LOGA(&data, "App: could not send");
        return 0;
    }
}

PROCESS(app_collect, "App colect process");
void app_start(){
    process_start(&app_collect, NULL);
}
PROCESS_THREAD(app_collect, ev, data){
    static struct etimer periodic_timer;
    static struct etimer send_timer;
    static unsigned int cnt;
    static unsigned int to_send_cnt;
    static uint32_t seqno;

    PROCESS_BEGIN();
    simple_udp_register(&unicast_connection, UDP_PORT,
                         NULL, UDP_PORT, receiver);
    if(node_id != ROOT_ID) {
       etimer_set(&periodic_timer, SEND_INTERVAL);
       while (1) {
         etimer_set(&send_timer, random_rand() % (SEND_INTERVAL));
         PROCESS_WAIT_UNTIL(etimer_expired(&send_timer));
         if (default_instance != NULL) {
           to_send_cnt++;
           while (to_send_cnt > 0) {
             seqno = ((uint32_t) node_id << 16) + cnt;
             if (app_send_to(ROOT_ID, seqno, to_send_cnt)) {
               cnt++;
               to_send_cnt--;
             if (to_send_cnt > 0) {
               etimer_set(&send_timer, CLOCK_SECOND);
               PROCESS_WAIT_UNTIL(etimer_expired(&send_timer));
               }
             } else {
                break;
                }
           }
         } else {
            LOG("App: no DODAG\n");
            }
            PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
            etimer_reset(&periodic_timer);
            }
    }
       PROCESS_END();
}

