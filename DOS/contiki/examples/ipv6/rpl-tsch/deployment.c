//
// Created by Hieu Nguyen on 5/7/2018.
//

#include <contiki-net.h>
#include "contiki.h"
#include "net/rpl/rpl.h"
#include "net/link-stats.h"
#include "deployment-log.h"
#include "sys/node-id.h"
#include "deployment.h"
#include "net/ip/uip-debug.h"
#include "lib/random.h"
#include "stdio.h"

#define DEBUG DEBUG_PRINT
#define PRINTF(...) printf(__VA_ARGS__)

static uint16_t selected_list[MAX_NODE];
static uint16_t is_selected(int id);
//static uint8_t injector_done = 0;
static int injector;
static struct ctimer injector_timer;

uint16_t get_preferred_parent_etx(void){
    rpl_parent_t *preferred_parent;
    rpl_dag_t *dag;
    /*suppose that we have one instance*/
    dag = rpl_get_any_dag();
    if (dag != NULL){
        preferred_parent = dag->preferred_parent;
        if (preferred_parent != NULL){
            const struct link_stats *stats = rpl_get_parent_link_stats(preferred_parent);
            if (stats != NULL){
                return stats->etx;
            }
        }
    }
    return 0xffff;
}

const linkaddr_t *get_preferred_parent_addr(void){
    rpl_parent_t *preferred_parent;
    rpl_dag_t *dag;
    const linkaddr_t *preferred_parent_addr;
    /*get any dag that we have*/
    dag = rpl_get_any_dag();
    if (dag != NULL){
        preferred_parent = dag->preferred_parent;
        if (preferred_parent != NULL){
            preferred_parent_addr = rpl_get_parent_lladdr(preferred_parent);
            return preferred_parent_addr;
        }
    }
    return NULL;
}

void set_preferred_parent_etx(uint16_t new_etx){
    rpl_parent_t *preferred_parent;
    rpl_dag_t *dag;
    /*get any dag that we have*/
    dag = rpl_get_any_dag();
    if (dag != NULL){
        preferred_parent = dag->preferred_parent;
        if (preferred_parent != NULL){
            struct link_stats *stats = rpl_get_parent_link_stats(preferred_parent);
            stats->etx = new_etx;
        }
    }
}
static uint16_t is_selected(int id){
    int i;
    for (i = 0; i < MAX_NODE ; i++) {
        if ((id != 0) && (id == selected_list[i])){
            return 1;
        }
    }
    return 0;
}

uint8_t select_random_node(uint8_t seed){
    /*make sure all nodes have the same seed*/
    random_init(seed);
    int index = ((random_rand() >> 8) % MAX_NODE) + 1;
    if((!is_selected(index)) && (index != 1)){
        selected_list[index - 1] = index;
        return index;
    }
    /*select another node if it's chosen already*/
    return select_random_node(seed + MAX_NODE);
}

void inject_fail_links(uint8_t inject_interval){
    /*Inject failure link
           * new_link_etx = old_link_etx * 10 */
    const linkaddr_t *failed_node = get_preferred_parent_addr();
    for (injector = 0; injector < NODE_PER_INJECTION; injector++ ){
        uint8_t selected_node = select_random_node(253);
        PRINTF("Selected node %d\n", selected_node);
        if (failed_node->u8[7] == selected_node) {
            PRINTF("Failure link injection in node %d\n",selected_node );
            set_preferred_parent_etx((uint16_t)(get_preferred_parent_etx() * 10));
            //injector_done = 1;
        }
    }
    ctimer_set(&injector_timer, CLOCK_SECOND * 60 * inject_interval, inject_fail_links, inject_interval);
}



