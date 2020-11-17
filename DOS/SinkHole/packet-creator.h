/*
 * Copyright (C) 2015 SDN-WISE
 *
*/

#ifndef PACKET_CREATOR_H_
#define PACKET_CREATOR_H_

#define BEACON_HOPS_INDEX 0
#define BEACON_BATT_INDEX 1
#define REPORT_INIT_INDEX 2
#define OPEN_PATH_WINDOWS_INDEX 0

#include "packet-buffer.h"

/* packets API. */
  packet_t* create_beacon(void);
  packet_t* create_data(uint8_t);
  packet_t* create_report(int);
  packet_t* create_reg_proxy(void);
  void create_and_send_request(packet_t*);
  packet_t* create_config(void);

/*create_data with fixed length*/
  //void create_and_send_data(void*);
 void create_and_send_data(uint8_t);
  extern int sendclock;
#endif /* PACKET_CREATOR_H_ */
/** @} */
