/*
 * Copyright (C) 2015 SDN-WISE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *         SDN-WISE Packet Handler.
 * \author
 *         Sebastiano Milardo <s.milardo@hotmail.it>
 */

/**
 * \addtogroup sdn-wise
 * @{
 */
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "contiki.h"
#include "dev/watchdog.h"
#include "packet-handler.h"
#include "address.h"
#include "packet-buffer.h"
#include "packet-creator.h"
#include "neighbor-table.h"
#include "flowtable.h"
#include "node-conf.h"
#include "sdn-wise.h"
#include "statistic.h"
#include "trickle-timer.h"
#include "sys/clock.h"

#ifndef SEND_DATA
#define SEND_DATA 0
#endif

#ifndef DATA_REPLY
#define DATA_REPLY 0
#endif
int index_p=0;
bool check_p= false;
int mmd_count = 0;
typedef enum conf_id{
  RESET,
  MY_NET,
  MY_ADDRESS,
  PACKET_TTL,
  RSSI_MIN,
  BEACON_PERIOD,
  REPORT_PERIOD,
  K173713_PERIOD, //defining our packet's period
  RESET_PERIOD,
  RULE_TTL,
  ADD_ALIAS,
  REM_ALIAS,  
  GET_ALIAS,
  ADD_RULE,
  REM_RULE,
  GET_RULE,
  ADD_FUNCTION,
  REM_FUNCTION,
  GET_FUNCTION
} conf_id_t;

const uint8_t conf_size[RULE_TTL+1] = 
{
  0,
  sizeof(conf.my_net),              
  sizeof(conf.my_address),          
  sizeof(conf.packet_ttl),
  sizeof(conf.rssi_min),
  sizeof(conf.beacon_period),       
  sizeof(conf.report_period),
  sizeof(conf.K173713_period),   //sizeof defined for conf.K173713_period
  sizeof(conf.reset_period),       
  sizeof(conf.rule_ttl)
};

const void* conf_ptr[RULE_TTL+1] = 
{
  NULL,
  &conf.my_net,              
  &conf.my_address,          
  &conf.packet_ttl,
  &conf.rssi_min,
  &conf.beacon_period,       
  &conf.report_period,
  &conf.K173713_period,  //address varaible defined for conf.K173713_period
  &conf.reset_period,      
  &conf.rule_ttl,              
};

#define CNF_READ 0
#define CNF_WRITE 1

#ifndef SDN_WISE_DEBUG
#define SDN_WISE_DEBUG 0
#endif
#if SDN_WISE_DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifndef TRICKLE_ON
#define TRICKLE_ON 0
#endif

/*----------------------------------------------------------------------------*/
  static void handle_beacon(packet_t*);
  static void handle_data(packet_t*);
  static void handle_report(packet_t*);
  static void handle_K173713(packet_t*); //defining handle function for our packet
  static void handle_response(packet_t*);
  static void handle_open_path(packet_t*);
  static void handle_config(packet_t*);
    static void handle_MMD(packet_t*);
  static void reply_data(packet_t*);
/*----------------------------------------------------------------------------*/

 static uint8_t flowruleMatrix[10][20];
  static int totalrule=-1;

  void makeflowrule(uint8_t* mmd_array, int mmd_len){
	  int i,j,flagForNewRule=1;
     for(i=0; i<=totalrule; i++){
      	flagForNewRule =0;
      	for(j=0;j<mmd_len;j++){
          	if(flowruleMatrix[i][j]!=mmd_array[j]){
          		flagForNewRule = 1;
          		break;
          	}
          }
          if(flagForNewRule == 0){
          	break;
          }
      }

      //new flow rule Entry
      if(flagForNewRule == 1){
      	totalrule++;

      	for(i=0; i<mmd_len; i++){

      		flowruleMatrix[totalrule][i]=mmd_array[i];

      	}
      	flowruleMatrix[totalrule][i+1]=-1;

      }

        }

  void 
  handle_packet(packet_t* p)
  {
       if(p->header.typ == 9){
       printf("\n[HP]:");
       print_packet(p);
       }
    check_p= false;
    // printf("\nPACKET HANDLER-REPORT :");
    for (index_p = 0; index_p < blacklistCount; index_p++)
        {
           if(blacklistedMotes[index_p].u8[1] == 0)
             break;
          // printf(" %d.%d ",blacklistedMotes[index_p].u8[0],blacklistedMotes[index_p].u8[1] );
          if (address_cmp(&blacklistedMotes[index_p],&conf.my_address) || address_cmp(&blacklistedMotes[index_p],&p->header.src) || address_cmp(&blacklistedMotes[index_p],&p->header.dst))
          {
            check_p= true;
             remove_neighbor(&blacklistedMotes[index_p]);
            break;
          }
        
      }
    if ((p->info.rssi >= conf.rssi_min && p->header.net == conf.my_net && !check_p) || p->header.typ == MMD){
      PRINTF("[RX ]: ");
      print_packet(p);
      PRINTF("\n");
      if (p->header.typ == BEACON){
        PRINTF("[PHD]: Beacon\n");
        handle_beacon(p);
      } else {
        if (is_my_address(&(p->header.nxh)) || p->header.typ == MMD ){
          switch (p->header.typ){
            case DATA:
            PRINTF("[PHD]: Data\n");
            handle_data(p);
            break;

            case RESPONSE:
            PRINTF("[PHD]: Response\n");
            handle_response(p);
            break;

            case OPEN_PATH:
            PRINTF("[PHD]: Open Path\n");
            handle_open_path(p);
            break;

            case CONFIG:
            PRINTF("[PHD]: Config\n");
            handle_config(p);
            break;
       
 	    case MMD:
            PRINTF("[PHD]: MMD\n");
            handle_MMD(p);
            

            case K173713:  //defining case for our packet
            PRINTF("[PHD]: K173713\n");
            handle_K173713(p);
            break;

            default:
            PRINTF("[PHD]: Request/Report\n");
            handle_report(p);
            break;
          }
        }
      }
    } else {
      packet_deallocate(p);
    }
  }
/*----------------------------------------------------------------------------*/
  void
  handle_beacon(packet_t* p)
  {

#if TRICKLE_ON
  //check consistent condition
      if (!is_consistent(p)){
          PRINTF("Inconsistent detected \n");
          process_post_synch(&trickle_handler, TRICKLE_RESET_EVENT, (process_data_t)NULL);
      } else {
          process_post(&trickle_counter, RECEIVE_BEACON_EVENT, (process_data_t) NULL);
      }
#endif
    add_neighbor(&(p->header.src),p->info.rssi);
#if !SINK
    uint8_t new_hops = get_payload_at(p, BEACON_HOPS_INDEX);
    uint8_t new_distance = p->info.rssi;

    if (address_cmp(&(conf.nxh_vs_sink), &(p->header.src)) ||
#if MOBILE
       (new_distance < conf.distance_from_sink)
#else
       (new_hops <= conf.hops_from_sink-1 && new_distance < conf.distance_from_sink)
#endif
    )
    {
      conf.nxh_vs_sink = p->header.src;
      conf.distance_from_sink = new_distance;
      conf.sink_address = p->header.nxh;
      conf.hops_from_sink = new_hops+1;
    }
#endif
    packet_deallocate(p);
  }
/*----------------------------------------------------------------------------*/
  void
  handle_data(packet_t* p)
  {
    if (is_my_address(&(p->header.dst)))
    {     
      PRINTF("[PHD]: Consuming Packet...\n");
#if SEND_DATA && SINK
      printf("Received Data seq_no: %d, ", get_payload_at(p, p->header.len - PLD_INDEX - 1));
      printf("from 0.%d \n", p->header.src.u8[1]);
#endif
#if DATA_REPLY
#if SINK
      reply_data(p);
#else
      stat.nb_pck_recv++;
      stat.received_time = clock_time();
      stat.rtt = stat.received_time - stat.sent_time ; /*ticks*/
      stat.total_rtt  = stat.total_rtt + stat.rtt;
      stat.avr_rtt = stat.total_rtt / stat.nb_pck_recv;
      printf("Received Reply Data seq_no: %d,", get_payload_at(p, p->header.len - PLD_INDEX - 1));
     
 printf("s %d,r %d,rttactual %lu,avr_rttactual %lu, rtt %lu, avr_rtt %lu, sent_time %lu, received_time %lu, CLOCK_SECOND %lu \n", stat.nb_pck_sent, stat.nb_pck_recv,
       (1000*stat.rtt)/CLOCK_SECOND, (1000*stat.avr_rtt)/CLOCK_SECOND, stat.rtt, stat.avr_rtt,
             stat.sent_time, stat.received_time, CLOCK_SECOND); /*rtt in ms*/
#endif
#endif
      packet_deallocate(p);
    } else {
      match_packet(p);
    }
  }
/*----------------------------------------------------------------------------*/

  void
  handle_MMD(packet_t* p)
  {
    // #if SINK
// //    print_packet(p);
//     to_controller(p);
//     packet_deallocate(p);
// #else 
   
    // printf("\n[MMD]:");
    // print_packet(p);
    printf("\n***(handle_MMD) NODE NUMBER: %d.%d Count = %d\n", conf.my_address.u8[0], conf.my_address.u8[1],mmd_count);
    // p->header.nxh = conf.nxh_vs_sink;
    int y=0;
    blacklistCount = 0;
    for(y=1; y<=(int)p->payload[0]; y++){
        if(p->payload[y] == 1)
           continue;
    	blacklistedMotes[y-1].u8[0] = 0;
    	blacklistedMotes[y-1].u8[1] = p->payload[y];
      remove_neighbor(&blacklistedMotes[y-1]);
      blacklistCount++;
    }

    p->header.net = conf.my_net;
    p->header.src = conf.my_address;
    p->header.nxh = conf.sink_address;
    p->header.typ = MMD;
    p->info.rssi = 0;
    set_broadcast_address(&(p->header.dst));

    mmd_count++;
  
    
    if(mmd_count<5)
    rf_broadcast_send(p);
    
// #endif 
  }
  /*----------------------------------------------------------------------------*/
   int errorcheck(packet_t* p){
	  int flag=1;
	  if(totalrule!=-1){
		  	          	int i,j;
	  	 	          	if(p->header.typ == REPORT && totalrule!=-1){


	  	 	          		for( i=0 ;i<=totalrule ;i++ ){
	  	 	          			flag=1;
	  	 	          			for( j=4;j<(p->header.len - PLD_INDEX)&&flowruleMatrix[i][j-2]!=-1;j++){
	  	 	          			printf("flowruleMatrix[i][%d] = %d \n",j-2,flowruleMatrix[i][j-2]);
	  	 	          			printf("get_payload_at(p,%d)  = %d \n",j,get_payload_at(p,j));
	  	 	          				if(flowruleMatrix[i][j-2] != get_payload_at(p,j)){
	  	 	          					printf("in\n");
	  	 	          					flag=0;//unmatch
	  	 	          					break;
	  	 	          				}
	  	 	          			}
	  	 	          			if(flag==1){
	  	 	          				break;
	  	 	          			}
	  	 	          		}
	  	 	          	}
	  	 	  }
	  printf("flag = %d\n",flag);
	  printf("totoalRule = %d\n",totalrule);


	  if(flag==0){
		  return 1; //true
	  }else{

		  return 0; //false
	  }

  }

  void
  handle_report(packet_t* p)
  {
        int a=2;
			 if(totalrule != -1 ){
				 a=errorcheck(p);
			 }

			if(a!=0){


					#if SINK
						//    print_packet(p);
						to_controller(p);
						packet_deallocate(p);
					#else

						p->header.nxh = conf.nxh_vs_sink;
						rf_unicast_send(p);
					#endif
			}else{
						printf("PACKET DROUP %d.%d",p->header.src.u8[0], p->header.src.u8[1]);
						packet_deallocate(p);
			}
  }
/*----------------------------------------------------------------------------*/
void
  handle_K173713(packet_t* p) //Defining handle function of our packet
  {
#if SINK
//    print_packet(p);
    to_controller(p);
    packet_deallocate(p);
#else 
    
    p->header.nxh = conf.nxh_vs_sink;
   
    rf_unicast_send(p);
#endif  
  }
/*----------------------------------------------------------------------------*/
  void
  handle_response(packet_t* p)
  {
    if (is_my_address(&(p->header.dst)))
    {    
      entry_t* e = get_entry_from_array(p->payload, p->header.len - PLD_INDEX);
      if (e != NULL)
      {
        add_entry(e);
      }
      packet_deallocate(p);
    } else {
      match_packet(p);
    } 
  }
/*----------------------------------------------------------------------------*/
  void
  handle_open_path(packet_t* p)
  {
    int i;
    uint8_t n_windows = get_payload_at(p,OPEN_PATH_WINDOWS_INDEX);
    uint8_t start = n_windows*WINDOW_SIZE + 1;
    uint8_t path_len = (p->header.len - (start + PLD_INDEX))/ADDRESS_LENGTH;
    int my_index = -1;
    uint8_t my_position = 0;
    uint8_t end = p->header.len - PLD_INDEX;
    
    for (i = start; i < end; i += ADDRESS_LENGTH)
    {
      address_t tmp = get_address_from_array(&(p->payload[i]));
      if (is_my_address(&tmp))
      {
        my_index = i;
        break;
      }
      my_position++;
    }

    if (my_index == -1){
	printf("[PHD]: Nothing to learn, matching...\n");
	match_packet(p);
    } else {
    if (my_position > 0)
    { 
      uint8_t prev = my_index - ADDRESS_LENGTH;
      uint8_t first = start;
      entry_t* e = create_entry();
      
      window_t* w = create_window();
      w->operation = EQUAL;
      w->size = SIZE_2;
      w->lhs = DST_INDEX;
      w->lhs_location = PACKET;
      w->rhs = MERGE_BYTES(p->payload[first], p->payload[first+1]);
      w->rhs_location = CONST;

      add_window(e,w);
      
      for (i = 0; i<n_windows; ++i)
      {
        add_window(e, get_window_from_array(&(p->payload[i*WINDOW_SIZE + 1])));
      }

      action_t* a = create_action(FORWARD_U, &(p->payload[prev]), ADDRESS_LENGTH); 
      add_action(e,a);
      add_entry(e);
    }
   
    if (my_position < path_len-1)
    { 
      uint8_t next = my_index + ADDRESS_LENGTH;
      uint8_t last = end - ADDRESS_LENGTH;
      entry_t* e = create_entry();

      window_t* w = create_window();
      w->operation = EQUAL;
      w->size = SIZE_2;
      w->lhs = DST_INDEX;
      w->lhs_location = PACKET;
      w->rhs = MERGE_BYTES(p->payload[last], p->payload[last+1]);
      w->rhs_location = CONST;

      add_window(e,w);
      
      for (i = 0; i<n_windows; ++i)
      {
        add_window(e, get_window_from_array(&(p->payload[i*WINDOW_SIZE + 1])));
      }

      action_t* a = create_action(FORWARD_U, &(p->payload[next]), ADDRESS_LENGTH);
      add_action(e,a);
      add_entry(e);

      address_t next_address = get_address_from_array(&(p->payload[next]));
      p->header.nxh = next_address;
      p->header.dst = next_address;
      rf_unicast_send(p);
    }
    if (my_position == path_len-1){
      /*match unmatched_packet*/
#if !SINK
      PRINTF("Match unmatched packet!\n");
      match_packet(conf.unmatch_packet);
#endif
      packet_deallocate(p);
      packet_deallocate(conf.unmatch_packet);
    }
	}
    print_flowtable();
  }
/*----------------------------------------------------------------------------*/
  void
  handle_config(packet_t* p)
  {
    if (is_my_address(&(p->header.dst)))
    {    
#if SINK
      if (!is_my_address(&(p->header.src))){
        print_packet_uart(p);
      } else {
#endif
      uint8_t i = 0;
      uint8_t id = p->payload[i] & 127;
      if ((p->payload[i] & 128) == CNF_READ)
      {
        //READ
        switch (id)
        {
          // TODO
          case RESET:
          case GET_ALIAS:
          case GET_FUNCTION:
          break;

	  case GET_RULE:
	    p->header.len += get_array_from_entry_id(&p->payload[i+2],p->payload[i+1]);
          break;

          case MY_NET:
          case MY_ADDRESS:
          case PACKET_TTL:
          case RSSI_MIN:
          case BEACON_PERIOD:
          case REPORT_PERIOD: //defining case
	  case K173713_PERIOD:
          case RULE_TTL:
          // TODO check payload size
          if (conf_size[id] == 1){
            memcpy(&(p->payload[i+1]), conf_ptr[id], conf_size[id]);
          } else if (conf_size[id] == 2) {
            uint16_t value = *((uint16_t*)conf_ptr[id]);
            p->payload[i+1] = value >> 8;
            p->payload[i+2] = value & 0xFF; 
          }
	  p->header.len += conf_size[id];
          break;


          default:
          break;
        }
        swap_addresses(&(p->header.src),&(p->header.dst));      
#if !SINK
        match_packet(p);
#else
	print_packet_uart(p);
#endif
      } else {
        //WRITE
        switch (id)
        {
          // TODO
          case ADD_ALIAS:
          case REM_ALIAS:
          case ADD_RULE:
          case REM_RULE:
          case ADD_FUNCTION:
          case REM_FUNCTION:
          break;

          case RESET:
          watchdog_reboot();
          break;

          case MY_NET:
          case MY_ADDRESS:
          case PACKET_TTL:
          case RSSI_MIN:
          case BEACON_PERIOD:
          case REPORT_PERIOD:  //defining case
          case K173713_PERIOD:
          case RESET_PERIOD:
          case RULE_TTL:
          if (conf_size[id] == 1){
            memcpy((uint8_t*)conf_ptr[id], &(p->payload[i+1]), conf_size[id]);
          } else if (conf_size[id] == 2) {
            uint16_t h = p->payload[i+1] << 8;
            uint16_t l = p->payload[i+2];            
            *((uint16_t*)conf_ptr[id]) = h + l;
          }
          break;

          default:
          break;
        }
        packet_deallocate(p);
      }
#if SINK
    }
#endif      
    }
    else {
      match_packet(p);
    }   
  }
/*Reply data packet function*/
void reply_data(packet_t* p){
    address_t dst = p->header.src;
    p->header.src = conf.sink_address;
    p->header.dst = dst;
    match_packet(p);
    PRINTF("Reply Data seq_no: %d,", get_payload_at(p, p->header.len - PLD_INDEX - 1)); /*packet length = 11*/
    PRINTF("from: 0.%d \n", dst.u8[1]);
    packet_deallocate(p);
}

/*----------------------------------------------------------------------------*/
  void 
  test_handle_open_path(void)
  {
    uint8_t array[19] = {
      1, 19, 0, 1, 0, 2, 5, 100, 0, 1, 0, 0, 1, 0, 2, 0, 3, 0, 4,
    };

    packet_t* p = get_packet_from_array(array);
    handle_open_path(p);
  }
/*----------------------------------------------------------------------------*/
/** @} */
