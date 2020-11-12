import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.util.ArrayList;

/**
 * Created by ABD.
 */
public class ScriptSDNWISE {
    /*
* 01:01.745	ID:12	Send Data seq_no: 0
* 01:01.878	ID:3	Send Data seq_no: 0
* 01:03.438	ID:1	Received Data seq_no: 0, from 0.9
* 01:03.658	ID:1	Received Data seq_no: 0, from 0.10
* */

    public static void main(String args[]){
        ScriptSDNWISE s = new ScriptSDNWISE();
        s.loadFile(s.inputFile);
        ArrayList<Integer> pdr = s.getNodePDR(3);
       // System.out.println("Sent: " + pdr.get(0) + " Received: " + pdr.get(1));
      //  s.printLogs(15, "Received Data" );
     //   System.out.println("Total Sent: " + s.getTotalPDR().get(0) + " Total Received: " + s.getTotalPDR().get(1));
         // System.out.println("delay of all delays of mote = " + delay);
          //System.out.println("total delay after adding all delays of mote = " + totalDelay);
        
        System.out.println("Average delay " + s.getAverageDelay());
        
    }

    String seqNumber;
    public Object lines[];
    public BufferedReader b;
    private int networkSize =8;
    private String inputFile = "/home/abdullah/Desktop/performance code check/EXPERIMENTS OF THESIS/SCENARIO 3/MULTI HOP TOPOLOGY 2/8 Motes/exp2/loglistener milisec time.txt";

    /**Load input file and parse lines object*/
    private void loadFile(String input){
        File in = new File(input);
        try {
            b = new BufferedReader(new FileReader(in));
            this.lines = b.lines().toArray();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        int a1 = 0;
        int a2 = 0;
        int a3 = 0;
        int a4 = 0;
        for (int i = 0; i < lines.length; i++){
            String currLine = lines[i].toString();
            if (currLine.contains("Send Data")){
                a1++;
            }
            if (currLine.contains("Received Data") ){
                a2++;
            }
            if (currLine.indexOf("Reply Data seq_no:") == 15 ){
                a3++;
            }
            if (currLine.contains("Received Reply Data") ){
                a4++;
            }
        }
        System.out.println("Total Sequence Sent: " + a1 + " and Total Sequence Received: " + a2 + "\nTotal Reply Data Sequence: " + a3 + " and Total Received Data Sequence: " + a4);
        
        //packet_delivery_ratio = received_packets/generated_packets*100;
        String s_sent = "" + a1;
        String s_recv = "" + a2;
        double pkt_del_rat = (Double.parseDouble(s_recv) / Double.parseDouble(s_sent)) * 100;
        System.out.println("Packet Delivery Ratio: " + pkt_del_rat);
        
      double numpkt_loss_rat = ( Double.parseDouble(s_sent) - Double.parseDouble(s_recv));

      double pktlossrate  =  (numpkt_loss_rat / Double.parseDouble(s_sent)) * 100;
      
      System.out.println("Packet Loss Ratio: " + pktlossrate);
      
       
        
        for(int m = 1; m <= networkSize; m++) {
            long delay = 0;
            int sent_seq = 0;
            long sentTimeStamp = 0;
            long recvTimeStamp = 0;
            for (int i = 0; i < lines.length; i++) {
                String currLine_i = lines[i].toString();
                if (currLine_i.contains("Send Data") && (getSenderID(currLine_i) == m)) {
                    sentTimeStamp = getTime(currLine_i);
                    sent_seq = getSeqNumber(currLine_i, true);
                }
               
                if (currLine_i.contains("Received Data") && (getReceiverID(currLine_i) == m) ) {
                    recvTimeStamp = getTime(currLine_i);
                    int recv_seq = getSeqNumber(currLine_i, false);
                    if (recv_seq == sent_seq) {
                        delay = recvTimeStamp - sentTimeStamp;
                       
                        System.out.println("send time of packet: " + m + " is: " + sentTimeStamp);
                       System.out.println("receive time of packet: " + m + " is: " + recvTimeStamp);
                        
                        System.out.println("Delay of Mote every data packet send: " + m + " is: " + delay);
                         
                    }
                }
            }
           
          //  System.out.println("Average Packet Delay of Mote: " + m + " is: " + delay);
        }
    }

    /**Get total PDR*/ 


// packet_delivery_ratio = received_packets/generated_packets*100;


    private ArrayList getTotalPDR(){
        int totalSent = 0, totalRecv = 0;
        ArrayList<Integer> totalPDR = new ArrayList<>();
        for (int i = 1; i<networkSize; i++){
            ArrayList<Integer> currPDR = getNodePDR(i+1);
            totalSent = totalSent + currPDR.get(0);
            totalRecv = totalRecv + currPDR.get(1);
        }
        totalPDR.add(totalSent);
        totalPDR.add(totalRecv);
        return totalPDR;
    }
    /**Get average delay of all nodes*/
    private long getAverageDelay(){
        long totalDelay = 0;
      
        for (int i = 1; i < networkSize; i++){
            totalDelay = totalDelay + getNodeDelay(i+1);
            
         
         //  System.out.println("total delay after adding all delays of mote = " + getNodeDelay(i+1));
        }
        return totalDelay / networkSize;
        
        
    }
    /**Return node PDR
     * [number_of_received_packet; number_of_sent_packet]*/
    private ArrayList<Integer> getNodePDR(int nodeID){
        ArrayList<Integer> nodePDR = new ArrayList<>();
        int nbOfSentPacket = 0;
        int nbOfReceivedPacket = 0;
        for (int i = 0; i < lines.length; i++){
            String currLine = lines[i].toString();
            if (currLine.contains("Send Data") && getSenderID(currLine) == nodeID){
                nbOfSentPacket++;
            }
            if (currLine.contains("Received Data") && getReceiverID(currLine) == nodeID){
                nbOfReceivedPacket++;
            }
        }
        nodePDR.add(nbOfSentPacket);
        nodePDR.add(nbOfReceivedPacket);
        return nodePDR;
    }

    /**Get node delay in ms*/
    private long getNodeDelay(int nodeID) {
        /* 153550	ID:2	Send Data seq_no: 1,clk 19614,t 153234
           153563	ID:1	Received Data seq_no: 1, from 0.2*/
        
        long totalDelay = 0, averageDelay = 0;
        long recvCount = 0;
        long delay = 0;
        int sent_seq = 0;
        long sentTimeStamp = 0;
        for (int i = 0; i < lines.length; i++) {
            String currLine_i = lines[i].toString();
            if (currLine_i.contains("Send Data") && (getSenderID(currLine_i) == nodeID)){
                sentTimeStamp = getTime(currLine_i);
                sent_seq = getSeqNumber(currLine_i, true);
            }
            //String currLine_j = lines[i].toString();
            if (currLine_i.contains("Received Data") && (getReceiverID(currLine_i) == nodeID)){
                long recvTimeStamp = getTime(currLine_i);
                int recv_seq = getSeqNumber(currLine_i, false);
                if (recv_seq == sent_seq){
                    delay = recvTimeStamp - sentTimeStamp;
                    recvCount++;
                    totalDelay = totalDelay + delay;
                    
                   // System.out.println("delay of all delays of mote = " + delay);
                  // System.out.println("total delay after adding all delays of mote = " + totalDelay);
                }
            }

        }
        if(totalDelay == 0 || recvCount == 0){
            return 0;
        }
        averageDelay = totalDelay / recvCount;
        System.out.println("averageDelay=totaldelay/recvcount = " + averageDelay);
        return averageDelay;
    }

    /**Get node ID of sender*/
    private int getSenderID(String currentLine){
        String senderID = currentLine.substring(currentLine.indexOf("Send Data")-3,
                currentLine.indexOf("Send Data")+4).replaceAll("\\D+","");
        return Integer.parseInt(senderID);
    }

    /**Get node ID of receiver*/
    private int getReceiverID(String currentLine){
        String receiverID = currentLine.substring(currentLine.indexOf("from")+7,
                currentLine.length()-1).replaceAll("\\D+","");
        return Integer.parseInt(receiverID);
    }

    /**Get time*/
    private long getTime(String currentLine){
        String timeStamp = currentLine.substring(0,
                currentLine.indexOf("ID")-1).replaceAll("\\D+","");
        return Long.parseLong(timeStamp);
    }

    /**Get sequence number*/
    private Integer getSeqNumber(String currentLine, boolean isSender){
        /*153550	ID:2	Send Data seq_no: 1,clk 19614,t 153234
          153563	ID:1	Received Data seq_no: 1, from 0.2
          60586	ID:24	Send Data seq_no: 1,clk 7716,t 60281*/

        if (isSender){
            seqNumber = currentLine.substring(currentLine.indexOf("seq_no")+8,
                    currentLine.indexOf(",clk")).replaceAll("\\D+","");
        }
        else {
            seqNumber = currentLine.substring(currentLine.indexOf("seq_no")+8,
                    currentLine.indexOf(",")).replaceAll("\\D+","");
        }
        return Integer.parseInt(seqNumber);
    }

    /**Print specific line*/
    private void printLogs(int nodeID, String regex){
        for (Object currLine: lines) {
            if (currLine.toString().contains(regex) && getReceiverID(currLine.toString()) == nodeID){
                System.out.println(currLine.toString());
            }
        }
    }
}
