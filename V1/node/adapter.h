#ifndef ADAPTER_H
#define ADAPTER_H

#include "contiki.h"
#include "packet-creator.h"
/*Send packet to the controller*/
void to_controller(packet_t*);
PROCESS_NAME(adapter_proc);
#endif //ADAPTER_H
