#ifndef SDN_WISE_H_
#define SDN_WISE_H_

#include "lib/list.h"
#include "lib/memb.h"
#include "packet-buffer.h"

void rf_unicast_send(packet_t*);
void rf_broadcast_send(packet_t*);

#endif