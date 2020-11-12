 //
// 
//
#include "contiki.h"
#include <stdio.h>
#include "adapter.h"
#include "dev/serial-line.h"
#include "packet-handler.h"
#include "packet-buffer.h"

#define PRINTF(...) printf(__VA_ARGS__)

void to_controller(packet_t* p){
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
}

PROCESS(adapter_proc, "Adapter process" );
AUTOSTART_PROCESSES(&adapter_proc);
PROCESS_THREAD(adapter_proc, ev, data){
    PROCESS_BEGIN();
        while (1){
            PROCESS_WAIT_EVENT();
            char *line;
            int index;
            line = (char *) data;
            if (ev == serial_line_event_message){
                
                printf("String From Controller: %s\n", line);
                printf("line[0]: %c\n", line[0]);
                printf("line[1]: %c\n", line[1]);
                printf("line[2]: %c\n", line[2]);

                int i, LEN;

                if(line[0] == '9'){
                printf("MMD: %s\n",line);
                int MMDLength = getMMdLength(line);

                printf("MMD Length %d\n",MMDLength);

                uint8_t MMDArray[MMDLength];

                // write to mmdarray by reference
                createMMDArray(line,MMDArray,MMDLength);

                printf("MMD Array: ");
                for(index = 0;index < MMDLength;index++){
                    printf("%d ",MMDArray[index]);
                }                
                printf("\n");

                // GET FUNCTION IS IN packetbuffer.h
                packet_t *MMDPacket = getMMDFromArray(MMDArray,MMDLength);
                // HANDLER DEFINED IN packet-handler.c 
                handle_packet(MMDPacket);

            }
            else
            {
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
                handle_packet(p);
            }
            }
           
        }
    PROCESS_END();
}

// 

int getMMdLength(char * MMD){
    int index = 0,MMDcount = 0;
    for(index = 0; MMD[index]!='\0';index++){
        if(MMD[index]==' ')
        MMDcount++;
    }
    return MMDcount;
}

void createMMDArray(char *line,uint8_t *MMDArray,int MMDLength){
    // char numArr[2] = '--'  concatenate from second index

    char numArr[] = "--";
    int charCountNumIn = 0, MMDcount = 0 , index = 0;

    // Giving warning for undefined reference to caller
    // therefore calloc malloc cant be used
    // using static char array instead of num array

    for(index = 0; line[index] != '\0'; index++){
        
        if(line[index] == ' '){
        charCountNumIn = 0;
        printf("num array: %s",numArr);
        if(numArr[1]=='-'){
            MMDArray[MMDcount] = (uint8_t)numArr[0]-48;
            MMDcount++;
        }
        else{
            MMDArray[MMDcount] = (uint8_t)atoi(numArr);
            MMDcount++;
        }
        numArr[0]= '-';
        numArr[1]= '-';
            }
    
    else
    {
            numArr[charCountNumIn] = line[index];
            charCountNumIn++;    
    }
    }
}