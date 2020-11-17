//
// .
//
#ifndef TRICKLE_DEBUG
#define TRICKLE_DEBUG 0
#endif

#if TRICKLE_DEBUG
#include <stdio.h>
#include <net/net-debug.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#include <lib/random.h>
#include <stdio.h>
#include "contiki.h"
#include "packet-creator.h"
#include "trickle-timer.h"
#include <inttypes.h>
#include <lib/trickle-timer.h>
#include "sdn-wise.h"
#include "neighbor-table.h"
#include "packet-buffer.h"
#include "node-conf.h"
#include "math.h"

/*Beacon callback functions*/
static void beacon_callbacks(void *ptr){
    static uint32_t sqn_beacon;
    sqn_beacon++;
    PRINTF("Send Beacon seq_no: %d ", sqn_beacon);
    PRINTF("at t = %" PRIu32 "(ticks)\n", (uint32_t)ptr);
    rf_broadcast_send(create_beacon());
}

/*Report callback fuctions*/
static void report_callbacks(void *ptr){
    static uint32_t sqn_report;
    sqn_report++;
    PRINTF("Send Report seq_no: %d", sqn_report);
    PRINTF(" at t = %" PRIu32 "(ticks)\n", (uint32_t)ptr );
    rf_unicast_send(create_report(0));
//TODO set default lifetime
}

/*Check consistent condition*/
//TODO add remove neighbor event
uint8_t is_consistent(packet_t* p){
    neighbor_t* n = neighbor_table_contains(&p->header.src);
    if (n == NULL){
        return 0;
    } else {
        double rssi_abs = fabs(p->info.rssi - n->rssi);
        if (rssi_abs > RSSI_Res){
            PRINTF("rssi_abs = %d ",(uint8_t)rssi_abs );
            return 0;
        }
        return 1;
    }
}

/*Trickle timer variables*/
static struct trickle_sdnwise trickle_timer;
static struct etimer current_interval;
static uint32_t I_curr, Imax;

/*Trickle timer processes*/
PROCESS(trickle_handler, "Trickle handler process");
PROCESS(trickle_counter, "Trickle counter process");
AUTOSTART_PROCESSES(&trickle_handler, &trickle_counter);
/*Trickle handler process*/
PROCESS_THREAD(trickle_handler, ev, data)
{
    PROCESS_BEGIN();

                I_curr = (1UL << Imin); /*First interval: I_current = 4096 milliseconds*/
                Imax = (1UL << (Imin + IntervalDoubling)); /*I_max = 2^20(milliseconds) with IntervalDoubling=8*/
                while (1){
#if !SINK
                    if (!conf.is_active){
                        PROCESS_WAIT_UNTIL(ev == ACTIVATE_EVENT);
                        packet_t* p = (packet_t*) data;
                        PRINTF("Actived by: %d\n", p->header.src.u8[1]);
                    }
#endif
                    /*t_beacon and t_report in milliseconds*/
                    trickle_timer.t_beacon = (I_curr / 2) + (I_curr / 2 * (uint32_t)random_rand()) / RANDOM_RAND_MAX;
                    etimer_set(&current_interval, (I_curr * CLOCK_SECOND) / 1000);
                    ctimer_set(&trickle_timer.beacon_timer, (trickle_timer.t_beacon * CLOCK_SECOND) / 1000,
                            beacon_callbacks,(void *) ((trickle_timer.t_beacon * CLOCK_SECOND) / 1000));
                    trickle_timer.counter = 0;
                    /*Send Report packet at the listen-only period of each interval*/
                    if (I_curr > (1UL << Imin)){
                        trickle_timer.t_report = ((uint32_t)random_rand() * I_curr /2) / RANDOM_RAND_MAX;
                        ctimer_set(&trickle_timer.report_timer, (trickle_timer.t_report * CLOCK_SECOND) / 1000,
                                report_callbacks, (void *) ((trickle_timer.t_report * CLOCK_SECOND) / 1000));
                    }
                    PRINTF("Start new Interval: I_curr = %" PRIu32 "(s),", I_curr / 1000);
                    PRINTF("t_beacon = %" PRIu32 "(ticks),", (trickle_timer.t_beacon * CLOCK_SECOND) / 1000);
                    PRINTF("t_report = %" PRIu32 "(ticks),", (trickle_timer.t_report * CLOCK_SECOND) / 1000);
                    PRINTF("c = %d\n ", trickle_timer.counter);

                    PROCESS_YIELD();
                    switch (ev) {
                        case TRICKLE_RESET_EVENT:
                        {
                            if (I_curr != (1UL << Imin)) {
                                I_curr = (1UL << Imin);
                                ctimer_stop(&trickle_timer.beacon_timer);
                                ctimer_stop(&trickle_timer.report_timer);
                                PRINTF("Reset Trickle timer, Interval ends.\n");
                            } else {
                                PRINTF("I_curr = Imin -> does nothing \n");
                            }
                        }
                            break;
                        case PROCESS_EVENT_TIMER:
                        {
                            if (I_curr*2 < Imax ) {
                                I_curr = I_curr * 2;
                                PRINTF("Interval ends, double current interval.\n" );
                            } else {
                                I_curr = Imax;
                                PRINTF("I_curr <- Imax \n" );
                            }
                        }
                        break;
                    }
                }
    PROCESS_END();
}
/*Trickle counter process*/
PROCESS_THREAD(trickle_counter, ev, data){
    PROCESS_BEGIN();
                while (1){
                    PROCESS_WAIT_EVENT_UNTIL(ev == RECEIVE_BEACON_EVENT);
                    trickle_timer.counter++;
                    PRINTF("Received consistent packet, counter c = %d \n", trickle_timer.counter);
                    if (trickle_timer.counter == RedundancyConstant){
                        ctimer_stop(&trickle_timer.beacon_timer);
                        ctimer_stop(&trickle_timer.report_timer);
                        PRINTF("Suppress scheduled packet, counter c = %d\n",trickle_timer.counter);
                        trickle_timer.counter = 0;
                    }
                }
    PROCESS_END();
}
