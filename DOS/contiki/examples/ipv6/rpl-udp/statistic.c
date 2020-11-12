//
// Created by Hieu Nguyen on 3/27/2018.
//
#include <sys/clock.h>
#include "statistic.h"
#include "stdio.h"
node_statistic_t stat;

/*Calculate delay, time synchronization
 * https://sourceforge.net/p/contiki/mailman/message/34073597/
 */

void statistic_init(void){
    stat.nb_pck_sent = 0;
    stat.nb_pck_recv = 0;
    stat.rtt = 0;
    stat.avr_rtt = 0;
    stat.total_rtt = 0;
    stat.sent_time = 0;
    stat.received_time = 0;
}
void print_statistic(void){
            printf("Performance statistics: nb_pck_sent %d, nb_pck_recv %d,"
            "rtt %lu, avr_rtt %lu, sent_time %lu, received_time %lu, CLOCK_SECOND %lu\n",
             stat.nb_pck_sent, stat.nb_pck_recv, stat.rtt, stat.avr_rtt,
             stat.sent_time, stat.received_time, CLOCK_SECOND);
}

