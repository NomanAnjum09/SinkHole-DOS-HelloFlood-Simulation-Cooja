#include <dev/serial-line.h>
#include "contiki.h"
#include "net/rime/rime.h"
#include "net/linkaddr.h"
#include "dev/watchdog.h"
#include <stdbool.h>
//#include "dev/uart1.h"
#include "../contiki/cpu/msp430/dev/uart1.h"
#include "../contiki/cpu/msp430/dev/uart0.h"
#include "dev/leds.h"
#include "lib/list.h"
#include "trickle-timer.h"
#include "lib/random.h"
#if CFS_ENABLED
#include "cfs/cfs.h"
#endif
#if ELF_ENABLED
#include "loader/elfloader.h"
#endif
#include "sdn-wise.h"
#include "flowtable.h"
#include "packet-buffer.h"
#include "packet-handler.h"
#include "packet-creator.h"
#include "neighbor-table.h"
#include "node-conf.h"
#include "adapter.h"
#include "statistic.h"
#include "../contiki/cpu/cc2538/dev/uart.h"
#include "../contiki/apps/powertrace/powertrace.h"
#include "sys/node-id.h"
#include "packet-creator.h"
#define UART_BUFFER_SIZE MAX_PACKET_LENGTH

#define UNICAST_CONNECTION_NUMBER 29
#define BROADCAST_CONNECTION_NUMBER 30

#define TIMER_EVENT 50
#define RF_U_SEND_EVENT 51
#define RF_B_SEND_EVENT 52
#define RF_U_RECEIVE_EVENT 53
#define RF_B_RECEIVE_EVENT 54
#define UART_RECEIVE_EVENT 55
#define RF_SEND_BEACON_EVENT 56
#define RF_SEND_REPORT_EVENT 57
#define RF_SEND_DATA_EVENT 58
#define NEW_PACKET_EVENT 59
#define ACTIVATE_EVENT 60

int beacon_count = 0; 

#ifndef SDN_WISE_DEBUG
#define SDN_WISE_DEBUG 0
#endif
#if SDN_WISE_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#include <stdio.h>
#define PRINTF(...)
#endif

#ifndef TRICKLE_ON
#define TRICKLE_ON 0
#endif

#ifndef SEND_DATA
#define SEND_DATA 0
#endif
uint8_t pk_pointer_seq = 1;
#if SEND_DATA
#define BACK_OFF (60 * CLOCK_SECOND) //(random_rand() % (4 * CLOCK_SECOND))
static struct ctimer back_off_timer;
#endif
/*----------------------------------------------------------------------------*/
PROCESS(main_proc, "Main Process");
PROCESS(rf_u_send_proc, "RF Unicast Send Process");
PROCESS(rf_b_send_proc, "RF Broadcast Send Process");
PROCESS(packet_handler_proc, "Packet Handler Process");
PROCESS(timer_proc, "Timer Process");
PROCESS(beacon_timer_proc, "Beacon Timer Process");
PROCESS(report_timer_proc, "Report Timer Process");
PROCESS(data_timer_proc, "Data Timer Process");
PROCESS(data_packets_process, "Data Packets Process");

AUTOSTART_PROCESSES (
    &main_proc,
    &rf_u_send_proc,
    &rf_b_send_proc,
    &timer_proc,
    &beacon_timer_proc,
    &report_timer_proc,
#if SINK
    &adapter_proc,
#endif
#if !SINK
    &data_timer_proc,
#endif
    &packet_handler_proc,
    &data_packets_process
);

/*----------------------------------------------------------------------------*/
static uint8_t uart_buffer[UART_BUFFER_SIZE];
static uint8_t uart_buffer_index = 0;
static uint8_t uart_buffer_expected = 0;

int blacklistCount;
address_t blacklistedMotes[40];

bool check_i = false;
bool check_j = false;
bool check_k = false;
bool check_l = false;
int index_i;
int index_j;
int index_k;
int index_l;
#if MOBILE
static uint8_t count = 0;
static packet_t *p;
#endif
static uint8_t count = 1;
static packet_t *data_packet;
/*----------------------------------------------------------------------------*/
void rf_unicast_send(packet_t *p)
{
  check_l= false;
      printf("\nRF_UNICAST MMD :");
        for (index_l = 0; index_l <= blacklistCount; index_l++)
          {
            // printf(" %d.%d ",blacklistedMotes[index_l].u8[0],blacklistedMotes[index_l].u8[1] );
            if (address_cmp(&blacklistedMotes[index_l],&p->header.src) || address_cmp(&blacklistedMotes[index_l],&p->header.dst))
            {
              check_l= true;
              remove_neighbor(&blacklistedMotes[index_l]);
              break;
            }
        
      }
      if(!check_l){
  process_post(&rf_u_send_proc, RF_U_SEND_EVENT, (process_data_t)p);
      }
} 
/*----------------------------------------------------------------------------*/
void data_send_tocontroller()
{
  process_post(&data_packets_process, 86, (char *)"");
}
/*----------------------------------------------------------------------------*/
void rf_broadcast_send(packet_t *p)
{
  check_l= false;
      // printf("\nRF-BROADCAST MMD :");
        for (index_l = 0; index_l <= blacklistCount; index_l++)
          {
            // printf(" %d.%d ",blacklistedMotes[index_l].u8[0],blacklistedMotes[index_l].u8[1] );
            if (address_cmp(&blacklistedMotes[index_l],&p->header.src) || address_cmp(&blacklistedMotes[index_l],&p->header.dst))
            {
              check_l= true;
               remove_neighbor(&blacklistedMotes[index_l]);
              break;
            }
        
      }
      if(!check_l){
  process_post(&rf_b_send_proc, RF_B_SEND_EVENT, (process_data_t)p);
      }
}
/*----------------------------------------------------------------------------*/
static uint8_t
get_packet_rssi(uint16_t raw_value)
{
  // TODO the exact rssi value depends on the radio
  // http://sourceforge.net/p/contiki/mailman/message/31805752/
#if COOJA
  return (uint8_t)-raw_value;
#else
  return (uint8_t)raw_value;
#endif
}

/*----------------------------------------------------------------------------*/
static void
unicast_rx_callback(struct unicast_conn *c, const linkaddr_t *from)
{
  packet_t *p = get_packet_from_array((uint8_t *)packetbuf_dataptr());
  if (p != NULL)
  {
    p->info.rssi = get_packet_rssi(packetbuf_attr(PACKETBUF_ATTR_RSSI));
    process_post(&main_proc, RF_U_RECEIVE_EVENT, (process_data_t)p);
  }
}
/*----------------------------------------------------------------------------*/
static void
broadcast_rx_callback(struct broadcast_conn *c, const linkaddr_t *from)
{
  packet_t *p = get_packet_from_array((uint8_t *)packetbuf_dataptr());
  if (p != NULL)
  {
    p->info.rssi = get_packet_rssi(packetbuf_attr(PACKETBUF_ATTR_RSSI));
    process_post(&main_proc, RF_B_RECEIVE_EVENT, (process_data_t)p);
  }
}
/*----------------------------------------------------------------------------*/
int uart_rx_callback(unsigned char c)
{
  uart_buffer[uart_buffer_index] = c;
  if (uart_buffer_index == LEN_INDEX)
  {
    uart_buffer_expected = c;
  }
  uart_buffer_index++;
  if (uart_buffer_index == uart_buffer_expected)
  {
    uart_buffer_index = 0;
    uart_buffer_expected = 0;
    packet_t *p = get_packet_from_array(uart_buffer);
    if (p != NULL)
    {
      p->info.rssi = 0;
      process_post(&main_proc, UART_RECEIVE_EVENT, (process_data_t)p);
    }
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
static const struct unicast_callbacks unicast_callbacks = {
    unicast_rx_callback};
static struct unicast_conn uc;
static const struct broadcast_callbacks broadcast_callbacks = {
    broadcast_rx_callback};
static struct broadcast_conn bc;
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(main_proc, ev, data)
{
  PROCESS_BEGIN();
  //uart1_init(BAUD2UBR(115200));       /* set the baud rate as necessary */
  //uart1_set_input(uart_rx_callback);  /* set the callback function */
  uart1_set_input(serial_line_input_byte); /*work with Tmote sky*/
  //uart0_set_input(serial_line_input_byte); /*work with Z1*/

  node_conf_init();
    flowtable_init();
    packet_buffer_init();
    neighbor_table_init();
    address_list_init();
    leds_init();
    statistic_init();
    print_statistic();
    /*powertrace start*/
    powertrace_start(60 * CLOCK_SECOND);

#if SINK
  print_packet_uart(create_reg_proxy());
#endif
  while (1)
  {
    PROCESS_WAIT_EVENT();
    switch (ev)
    {
    case TIMER_EVENT:
      printf("Kill all processes! \n");
      process_exit(&data_timer_proc);
      process_exit(&main_proc);
      process_exit(&rf_u_send_proc);
      process_exit(&rf_b_send_proc);
      process_exit(&packet_handler_proc);
      process_exit(&trickle_counter);
      process_exit(&trickle_handler);
      process_exit(&timer_proc);
      powertrace_stop();
      break;

    case UART_RECEIVE_EVENT:
      leds_toggle(LEDS_GREEN);
      process_post(&packet_handler_proc, NEW_PACKET_EVENT, (process_data_t)data);
      break;

    case RF_B_RECEIVE_EVENT:
      leds_toggle(LEDS_YELLOW);
      if (!conf.is_active)
      {
        conf.is_active = 1;
        process_post(&beacon_timer_proc, ACTIVATE_EVENT, (process_data_t)NULL);
        process_post(&report_timer_proc, ACTIVATE_EVENT, (process_data_t)NULL);
        packet_t *p = (packet_t *)data;
        process_post(&trickle_handler, ACTIVATE_EVENT, (process_data_t)p);
       // process_post(&packet_handler_proc, NEW_PACKET_EVENT, (process_data_t)data);
        printf("***(RF_B_RECEIVE_EVENT) NODE NUMBER: %d.%d\n", conf.my_address.u8[0], conf.my_address.u8[1]);
      }
    case RF_U_RECEIVE_EVENT:
      
      process_post(&packet_handler_proc, NEW_PACKET_EVENT, (process_data_t)data);
      break;

    case RF_SEND_BEACON_EVENT:
    check_k= false;
      packet_t *pb = create_beacon();
      // printf("\nCREATE BEACON MMD:");
      for (index_k = 0; index_k <= blacklistCount; index_k++)
          {
            // printf(" %d.%d ",blacklistedMotes[index_k].u8[0],blacklistedMotes[index_k].u8[1] );
            if (address_cmp(&blacklistedMotes[index_k],&pb->header.src) ||address_cmp(&blacklistedMotes[index_k],&pb->header.dst) || address_cmp(&blacklistedMotes[index_k],&conf.my_address) )
            {
              check_k= true;
               remove_neighbor(&blacklistedMotes[index_k]);
              break;
            }
        
      }
      if(!check_k){
        rf_broadcast_send(pb);
        beacon_count++;
        break;
        }
      else{
        packet_deallocate(pb);
        break;
      }
    break;
    case RF_SEND_REPORT_EVENT:
      check_k= false;
      packet_t *p = create_report(beacon_count);
      // printf("\nCREATE REPORT MMD: ");
        for (index_k = 0; index_k <= blacklistCount; index_k++)
          {
            // printf(" %d.%d ",blacklistedMotes[index_k].u8[0],blacklistedMotes[index_k].u8[1] );
            if (address_cmp(&blacklistedMotes[index_k],&p->header.src) || address_cmp(&blacklistedMotes[index_k],&p->header.dst) || address_cmp(&blacklistedMotes[index_k],&conf.my_address) )
            {
              check_k= true;
               remove_neighbor(&blacklistedMotes[index_k]);
              break;
            }
        
      }
      if(!check_k){
        rf_unicast_send(p);
        
  #if !SINK
        if (conf.reset_period == 0)
        {
          conf.distance_from_sink = _MAX_DISTANCE;
          conf.reset_period = _RESET_PERIOD;
        }
        else
        {
          conf.reset_period--;
        }
  #endif
        break;
      }
      else{
        packet_deallocate(p);
        break;
      }
      
  
    case RF_SEND_DATA_EVENT: break;
    }
  }
  PROCESS_END();
}
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(rf_u_send_proc, ev, data)
{
  static linkaddr_t addr;
  PROCESS_EXITHANDLER(unicast_close(&uc);)
  PROCESS_BEGIN();

  unicast_open(&uc, UNICAST_CONNECTION_NUMBER, &unicast_callbacks);
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == RF_U_SEND_EVENT);
    packet_t *p = (packet_t *)data;
    check_j= false;
    // printf("\nRF-U_SEND PROC MMD: ");
    for (index_j = 0; index_j <= blacklistCount; index_j++)
      {
        // printf(" %d.%d ",blacklistedMotes[index_j].u8[0],blacklistedMotes[index_j].u8[1] );
        if (address_cmp(&blacklistedMotes[index_j],&p->header.src) || address_cmp(&blacklistedMotes[index_j],&p->header.dst) || address_cmp(&blacklistedMotes[index_j],&conf.my_address))
        {
          check_j= true;
           remove_neighbor(&blacklistedMotes[index_j]);
          PRINTF("\nTXU blacklisted!");
          break;
        }
    
      }
        if(!check_j){

          if (p != NULL)
          {
            p->header.ttl--;


            PRINTF("[TXU]: ");
            print_packet(p);
            PRINTF("\n");
            if (!is_my_address(&(p->header.dst)))
            {
              int i = 0;

              int sent_size = 0;

              if (LINKADDR_SIZE < ADDRESS_LENGTH)
              {
                sent_size = LINKADDR_SIZE;
              }
              else
              {
                sent_size = ADDRESS_LENGTH;
              }

              for (i = 0; i < sent_size; ++i)
              {
                addr.u8[i] = p->header.nxh.u8[(sent_size - 1) - i];
              }

              addr.u8[0] = p->header.nxh.u8[1];
              addr.u8[1] = p->header.nxh.u8[0];
              uint8_t *a = (uint8_t *)p;
              packetbuf_copyfrom(a, p->header.len);
              unicast_send(&uc, &addr);
            }
      #if SINK
            else
            {
              to_controller(p);
              // print_packet_uart(p);
            }
      #endif
            packet_deallocate(p);
          }

          }
          else
          {
            packet_deallocate(p);
          }
          
  }
  PROCESS_END();
}
/*----------------------------------------------------------------------------*/
void getdatapacket(int id, int sq) {
    printf("[RX ]: 1 20 0.%d 0.1 0 98 0.%d 0 0 0 0 0 0 0 0 0 %s\n", id, id, sq);
    printf("[PHD]: Data\n");
    printf("[PHD]: Consuming Packet...\n");
    printf("Received Reply Data seq_no: %s,", sq);
}
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(rf_b_send_proc, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&bc);)
  PROCESS_BEGIN();
  broadcast_open(&bc, BROADCAST_CONNECTION_NUMBER, &broadcast_callbacks);
  while (1)
  {

    PROCESS_WAIT_EVENT_UNTIL(ev == RF_B_SEND_EVENT);
    packet_t *p = (packet_t *)data;

    if (p != NULL)
    {
      p->header.ttl--;
      PRINTF("[TXB]: ");
      if(p->header.typ == MMD){
          printf("Broadcasting...");
          print_MMD(p);
        }
        else{
          print_packet(p);
        }
      PRINTF("\n");

      uint8_t *a = (uint8_t *)p;
      packetbuf_copyfrom(a, p->header.len);
      broadcast_send(&bc);
      packet_deallocate(p);
    }
  }
  PROCESS_END();
}
/*----------------------------------------------------------------------------*/
 PROCESS_THREAD(timer_proc, ev, data) {
    static struct etimer et;
    PROCESS_BEGIN();

    while(1) {
      etimer_set(&et, 3610 * CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      process_post(&main_proc, TIMER_EVENT, (process_data_t)NULL);
    }
    PROCESS_END();
  }
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(beacon_timer_proc, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();
  while (1)
  {
#if !SINK
    if (!conf.is_active)
    {
      PROCESS_WAIT_EVENT_UNTIL(ev == ACTIVATE_EVENT);
    }
#endif
    etimer_set(&et, conf.beacon_period * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    process_post(&main_proc, RF_SEND_BEACON_EVENT, (process_data_t)NULL);
  }
  PROCESS_END();
}
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(report_timer_proc, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();
  while (1)
  {
#if !SINK
    if (!conf.is_active)
    {
      PROCESS_WAIT_EVENT_UNTIL(ev == ACTIVATE_EVENT);
    }
#endif
    etimer_set(&et, conf.report_period * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    process_post(&main_proc, RF_SEND_REPORT_EVENT, (process_data_t)NULL);
  }
  PROCESS_END();
}
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(data_timer_proc, ev, data)
{
  static int startdatapacketgeneration = 0;
  static struct etimer et;
  PROCESS_BEGIN();
  while (1)
  {
   // int num = (rand() % 15) + 70; 
    etimer_set(&et, 60 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    if(startdatapacketgeneration > 1) {
      create_and_send_data(pk_pointer_seq);
    }
    startdatapacketgeneration = startdatapacketgeneration + 1;
  }
  PROCESS_END();
}

/*----------------------------------------------------------------------------*/
PROCESS_THREAD(packet_handler_proc, ev, data)
{
  PROCESS_BEGIN();
  while (1)
  {
    
    PROCESS_WAIT_EVENT_UNTIL(ev == NEW_PACKET_EVENT);
      packet_t *p = (packet_t *)data;
      check_i= false;
      // if(p->header.typ == 9){
      //  printf("\n[MMD]:");
      //  print_packet(p);
      // }

      //  if(p->header.typ == 1){
      //  printf("\n[BBD]:");
      //  print_packet(p);
      // }
      
      //printf("\nPROCESS_HANDLER PROC MMD: ");
        for (index_i = 0; index_i <= blacklistCount; index_i++)
          {
            if(blacklistedMotes[index_i].u8[1] == 0)
             break;
            //printf(" %d.%d ",blacklistedMotes[index_i].u8[0],blacklistedMotes[index_i].u8[1] );
            if (address_cmp(&blacklistedMotes[index_i],&conf.my_address) || address_cmp(&blacklistedMotes[index_i],&p->header.dst) || address_cmp(&blacklistedMotes[index_i],&p->header.src) || address_cmp(&blacklistedMotes[index_i],&p->header.dst) )
            {
              check_i= true;
              remove_neighbor(&blacklistedMotes[index_i]);
              break;
            }
        
      }
      if(!check_i){
      //printf("calling handle for %d",p->header.typ );
      handle_packet(p);
      }
      else
      {
      //   printf("calling deallocate for %d",p->header.typ );
        packet_deallocate(p);
      }
      
      
      
      
  }
  PROCESS_END();
}

static int avgroundtiptime = 1;
static int totalroundtiptime = 1;
static int receivedpacket = 1;

static struct broadcast_conn broadcast_packetsdata;

static void
broadcast_recv_packet(struct broadcast_conn *c, const linkaddr_t *from)
{
  char *line = (char*)packetbuf_dataptr();
#if !SINK
  if(strstr(line,"#") != NULL) {
    int j = 0;
    char split[3][5];
    int le = strlen(line);
    char str[le];
    sprintf(str,"%s",line);
    char delim[] = "#";
    char *ptr = strtok(str, delim);
    while (ptr != NULL)
    {
        sprintf(split[j], "%s", ptr);
        j = j + 1;
        ptr = strtok(NULL, delim);
    }
    char gopack[10];
    sprintf(gopack, "%d", node_id);
    if( strcmp(split[1], gopack) == 0 ) {
      int t_tim = ((int)clock_time() < 0) ? ((int)clock_time() * (-1)) : (int)clock_time();
      if(t_tim < 0) { t_tim = (t_tim * (-1)); }
      printf("Packet Timming -> (Sent At: %d, Received At: %d\n", sendclock, t_tim);
      //getdatapacket(node_id, split[0]);
      int roundtiptime = t_tim - sendclock;
      if(roundtiptime < 0) { roundtiptime = roundtiptime * (-1); }
      totalroundtiptime = totalroundtiptime + roundtiptime;
      if(totalroundtiptime < 0) { totalroundtiptime = totalroundtiptime * (-1); }
      avgroundtiptime = totalroundtiptime / receivedpacket;
      int aaa = ((roundtiptime * 1000) / CLOCK_SECOND);
      if(aaa < 0) { aaa = aaa * (-1); }
      int bbb = ((avgroundtiptime * 1000) / CLOCK_SECOND);
      if(bbb < 0){ bbb = bbb * (-1); }
      printf("s %s,r %d,rtt %d,avr_rtt %d\n", split[0], receivedpacket, aaa, bbb);
      receivedpacket = receivedpacket + 1;
    }
    return;
  }
#endif

#if SINK
  if(strstr(line,"#") != NULL) {
    return;
  }
  getfltpacket(from->u8[0], from->u8[1], line);
  char reply[15];
  sprintf(reply,"%s#%d", line, from->u8[0]);
  packetbuf_copyfrom(reply, strlen(reply));
  broadcast_send(&broadcast_packetsdata);
#endif
}
static const struct broadcast_callbacks broadcast_call_datapacket = {broadcast_recv_packet};

PROCESS_THREAD(data_packets_process, ev, data)
{
  static struct etimer et;
  PROCESS_EXITHANDLER(broadcast_close(&broadcast_packetsdata);)
  PROCESS_BEGIN();
  broadcast_open(&broadcast_packetsdata, 88, &broadcast_call_datapacket);
#if !SINK
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == 86);
    char seq_n[10];
    sprintf(seq_n, "%d", pk_pointer_seq);
    packetbuf_copyfrom(seq_n, strlen(seq_n) + 1);
    broadcast_send(&broadcast_packetsdata);
    pk_pointer_seq = pk_pointer_seq + 1;
  }
#endif
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
