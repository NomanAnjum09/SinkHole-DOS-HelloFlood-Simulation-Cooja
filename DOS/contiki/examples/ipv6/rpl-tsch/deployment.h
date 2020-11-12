//
// Created by Hieu Nguyen on 5/7/2018.
//
/*
 * Orchestra failure links injection testbed
 * */
#ifndef DEPLOYMENT_H
#define DEPLOYMENT_H

#include <net/linkaddr.h>
#include "net/ip/simple-udp.h"

#define MAX_NODE 25
#define NODE_PER_INJECTION 3
/*Injection time and injection interval in minutes*/
#define START_INJECTION_TIME 30
#define INJECTION_INTERVAL 15

/*Get ETX from preferred parent*/
uint16_t get_preferred_parent_etx(void);
/*Get address of preferred parent*/
const linkaddr_t *get_preferred_parent_addr(void);
/*Set ETX to the preferred parent for emulating failed links*/
void set_preferred_parent_etx(uint16_t new_etx);
/*Send data */
int app_send_to(uint16_t id, uint32_t seqno, unsigned int to_send_cnt);
/*Select a random node*/
uint8_t select_random_node(uint8_t seed);
/*Inject fail links with pre-defined period*/
void inject_fail_links(uint8_t inject_interval);
#endif //DEPLOYMENT_H
