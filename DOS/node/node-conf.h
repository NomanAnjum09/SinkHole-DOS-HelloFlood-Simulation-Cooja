#ifndef NODE_CONF_H_
#define NODE_CONF_H_

#include <stdint.h>
#include "address.h"
#include "packet-buffer.h"

#define _NET  1
#define _BEACON_PERIOD    10
#define _REPORT_PERIOD    20
#define _RESET_PERIOD     100
#define _RULE_TTL         100
#define _RSSI_MIN         0
#define _RSSI_MAX         255
#define _PACKET_TTL       100
#define _MAX_DISTANCE     _RSSI_MAX
#define _MIN_DISTANCE     0
  typedef struct node_conf_struct {
    uint8_t my_net;
    address_t my_address;
    uint8_t packet_ttl;
    uint8_t rssi_min;
    uint16_t beacon_period;
    uint16_t report_period;
    uint16_t reset_period;
    uint16_t rule_ttl; 
    address_t nxh_vs_sink;
    uint16_t distance_from_sink;
    uint8_t hops_from_sink;
    address_t sink_address;
    uint8_t is_active;
    uint8_t requests_count;
    packet_t* unmatch_packet;
  } node_conf_t;
  
  /*global variables*/
  extern node_conf_t conf;
  extern char *cmmdpacket;
  extern int blacklistCount;
  extern address_t blacklistedMotes[40];

  /* Node Configuration API. */
  void node_conf_init(void);
  void print_node_conf(void);
#endif /* NODE_CONF */
/** @} */
