#include "contiki.h"
#include <stdio.h>
#include <stdlib.h>
#include "adapter.h"
#include "dev/serial-line.h"
#include "packet-handler.h"
#include "packet-buffer.h"

#define PRINTF(...) printf(__VA_ARGS__)

int lengthMMD(char *mmd);
void createMMD_Array(char *line, uint8_t *mmd_array, int mmd_len);

// sends packet to controller
void to_controller(packet_t* p){ // called from packet-handler.c
    /** Rearrange packet format
     * p: 1 16 0 1 0 3 2 98 0 1 2 255 1 0 2 83
     * Sink -> Controller: 1 16 0.1 0.3 2 98 0.1 2 255 1 0 2 83
     * */
    int i;
    printf("Sink -> Controller: ");
    printf("%d %d %d.%d %d.%d %d %d %d.%d ",
           p->header.net, p->header.len, p->header.dst.u8[0], p->header.dst.u8[1],
           p->header.src.u8[0], p->header.src.u8[1], (uint8_t)p->header.typ, p->header.ttl,
           p->header.nxh.u8[0], p->header.nxh.u8[1]);
    for (i=0; i < (p->header.len - PLD_INDEX); ++i){
        printf("%d ",get_payload_at(p,i));
    }
    printf("\n");
    packet_deallocate(p);
}

// Entry point from controller
PROCESS(adapter_proc, "Adapter process" );
AUTOSTART_PROCESSES(&adapter_proc);
// From Controller
// data: char* array
// ev: event
PROCESS_THREAD(adapter_proc, ev, data){
    PROCESS_BEGIN();
        while (1){
            PROCESS_WAIT_EVENT();
            if (ev == serial_line_event_message){
                int i, LEN;
                char *line;
                line = (char *) data;
                printf("String From Controller1: %s\n", line);
                printf("line[0]: %c\n", line[0]);
                printf("line[1]: %c\n", line[1]);
                printf("line[2]: %c\n", line[2]);

                // ** THE ABOVE LINES PRINTS ON MOTEOUT FOR MMD**
                // 04:08.430	ID:1	String From Controller: 9 1 5  // 5 = blaclisted node
                // 04:08.431	ID:1	line[0]: 9      // type of mmd
                // 04:08.432	ID:1	line[1]:  
                // 04:08.433	ID:1	line[2]: 1      // sink number

                // MMD has node numbers of blacklisted node
                // if line doesnot contain MMD which means line[0] != '9'
                if(line[0] == '9'){
                      // issue 01: does not enter here, so checking line[0]
                    // issue 01: solved !!!  
                    // 04:08.434	ID:1	MMD: 9 1 5 ;
                    printf("MAlicious Nodes Detected\n");
                    printf("MMD: %s\n", line);

                    int mmd_len = lengthMMD(line); 
                   
                    
                    // 04:14.376	ID:1	MMD Length: 3
                    printf("MMD Length: %d\n", mmd_len);

                    // int vs uint8_t
                    // int is generally 4 byte, depends on the size of register
                    // for 64-bit and 32-bit machine, int is 4-byte
                    // for 64-bit, long is 8-byte and 32-bit machine, long is 4-byte
                    // uint8_t is 8-bit unsigned integer ranging from [0-127]

                    uint8_t mmd_array[mmd_len];
                    
                    // by reference writes to mmd_array
                    createMMD_Array(line, mmd_array, mmd_len);

                    // 03:34.470	ID:1	MMD ARRAY: 9 1 5 
                    printf("MMD ARRAY1: ");
                    for(i=0; i<mmd_len; i++){
                        printf("%d ", mmd_array[i]);
                    }
                    printf("\n");

                    // function defined in paket-buffer.c
                    packet_t *mmd_packet = get_MMD_from_array(mmd_array, mmd_len);
                    handle_packet(mmd_packet);	// defined in packet-handler.c

             
                }
                else if(line[0] == '8'){
                    printf("MMD: %s\n", line);
                    int mmd_len = lengthMMD(line);
                    printf("MMD Length: %d\n", mmd_len);
                    uint8_t mmd_array[mmd_len];
                    createMMD_Array(line, mmd_array, mmd_len);
                    printf("MMD ARRAY: ");
                    makeflowrule(mmd_array, mmd_len);
                    

                }
                else{
                           // line has numbers only so it is type casted to uint8_t
                    // without scrambled characters
                    uint8_t *packet = (uint8_t *) line;
                    LEN = packet[1] - 32;
                    uint8_t packet_array[LEN];

                    for (i = 0; i < LEN; i++){
                        packet_array[i] = packet[i] - (uint8_t)32;
                    }

                    packet_t *p = get_packet_from_array(packet_array);
                    printf("Received from controller: ");
                    for (i=0; i < LEN; i++){
                        printf("%d ", packet_array[i]);
                    }
                    printf("\n");
                    p->info.rssi = 0;
                    handle_packet(p);	// defined in packet-handler.c
                  
                }
            }//if(outside ev == serial_line_event_message);
        }
    PROCESS_END();
}

// returns the count of numbers
// 12 is considered as a single number
int lengthMMD(char *mmd){

    int i=0, num_count = 0;

    for(i=0; mmd[i] != '\0'; i++){
        if(mmd[i] == ' '){
            num_count++;
        }
    }
    return num_count;
}

void createMMD_Array(char *line, uint8_t *mmd_array, int mmd_len){
    // char num_arr[2] = "--"; // concatinates from 2nd index
    char num_arr[] = "--";
    int char_count_in_no = 0;
    int mmd_count = 0, i=0;

    // giving warning for undefined reference to calloc
    // hence cannot use, calloc, malloc, free
    // so used static char array instead for num_array
    for(i=0; line[i] != '\0'; i++){
        if(line[i] == ' '){
            // TODO: convert num_arr to uint8_t and assign to mmd_array
            char_count_in_no = 0;   // resets
            // 04:10.970	ID:1	num_array: 9-
            // 04:10.971	ID:1	num_array: 1-
            // 04:10.972	ID:1	num_array: 5-
            printf("num_array: %s\n", num_arr);
            // if single digit number
            if(num_arr[1] == '-'){
                mmd_array[mmd_count] = (uint8_t)num_arr[0] - 48;
                mmd_count++;
            }
            else{
                mmd_array[mmd_count] = (uint8_t)atoi(num_arr);
                mmd_count++;
            }
            num_arr[0] = '-';
            num_arr[1] = '-';
        }
        else{
            if(char_count_in_no == 0){
                num_arr[char_count_in_no] = line[i];
                char_count_in_no++;
            }
            else if(char_count_in_no == 1){
                num_arr[char_count_in_no] = line[i];
                char_count_in_no++;
            }
        }
    }
}
