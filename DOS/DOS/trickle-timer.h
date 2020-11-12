//
// 
//
/*Trickle algorithm implementation in SDN-WISE*/
#ifndef TRICKLE_TIMER_H
#define TRICKLE_TIMER_H

#include "contiki.h"
#include "packet-buffer.h"
#include "../contiki/core/sys/clock.h"

#define Imin 12 /*Imin = 2^12(miliseconds) */
#define IntervalDoubling 8 /*Imax = 2^8 * Imin*/
#define RedundancyConstant 6 /*k = 6*/
#define RSSI_Res 20 /*RSSI resolution*/
#define REPORT_LIFETIME 19 /* 2^19(miliseconds) = 8.7 (minutes)*/

#define ACTIVATE_EVENT        60
#define TRICKLE_RESET_EVENT 61 /*event should be posted when inconsistency occurs*/
#define RECEIVE_BEACON_EVENT 62 /*event should be posted when received consistent transmission*/

uint8_t is_consistent(packet_t*);
struct trickle_sdnwise;
struct trickle_sdnwise{
    struct ctimer beacon_timer, report_timer; /*schedule beacon, report transmission*/
    uint8_t counter; /*counter c*/
    uint32_t t_report, t_beacon; /*random chosen time t to send beacon/report packet */
};

PROCESS_NAME(trickle_handler);
PROCESS_NAME(trickle_counter);
#endif //TRICKLE_TIMER_H
