/*Adaptation layer for Sink*/
#ifndef ADAPTER_H
#define ADAPTER_H

#include "contiki.h"
#include "packet-creator.h"
/*Send packet to the controller*/
void to_controller(packet_t*);
int lengthMMD(char *mmd);
PROCESS_NAME(adapter_proc);
PROCESS_NAME(mmd_monitoring_process);
#endif //ADAPTER_H
