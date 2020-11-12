package controller;


import connection.SerialConsole;
import org.contikios.contiki.collect.MoteFinder;
import org.contikios.contiki.collect.SerialConnection;
import org.contikios.contiki.collect.SerialConnectionListener;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;



import javax.swing.*;
import java.awt.*;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.util.HashMap;
import java.util.Observable;
import java.util.Observer;
import java.util.Properties;
import java.util.TimerTask;
import java.util.Timer;
import java.lang.Math;


public class Controller implements Observer, SerialConnectionListener {
	
    String inComingData;
    JFrame window;
    JTextPane textPane;
    JScrollPane scrollPane;
    JTextField textField;
    JDesktopPane desktop;
    JButton button;
    NetworkGraph graph;
    double Transmission_range = 50;
 
    private SerialConsole serialConsole;
    private SerialConnection serialConnection;
    private static final String MAIN = "main";
    private boolean doSendInitAtStartup = false;
    private Properties configTable = new Properties();
    private boolean hasSerialOpened;
    private boolean hasStarted = false;
    private int networkSize;
    double [][]coordinates = new double[33][2];
    boolean got_coordinates = false;
    boolean[] malicious_motes_wormhole = new boolean[51];
    boolean[] malicious_motes_newflow = new boolean[51];
    boolean check_wormholes =false;
    boolean check_newflow = true;
    HashMap<String, Integer> neighbor_table_count = new HashMap<String, Integer>();
    HashMap<Integer, String> mote_checked = new HashMap<Integer, String>();
    HashMap<Integer, Integer> report_count = new HashMap<Integer, Integer>();
    boolean nf_detected = false;
    String malicious_neighbor_table = "";
    
    
    public Controller(){
        serialConsole = new SerialConsole(this, MAIN);
        graph = new NetworkGraph();
        
        Timer timer030 = new Timer();
    	timer030.scheduleAtFixedRate(new TimerTask() {

    		@Override
    		public void run() {
    			
    			check_wormholes = true;
    			
    		}
    	}, 40000, 40000);
    	
    	Timer timer060 = new Timer();
      	timer060.scheduleAtFixedRate(new TimerTask() {

      		@Override
      		public void run() {
      			
      			System.out.println("Updating coordinates at controller.java");
      			got_coordinates = false;
      			for(Integer key : mote_checked.keySet()) {
      			//	System.out.println("checking -> " + Integer.toString(key));
      				if(mote_checked.get(key).equals(malicious_neighbor_table)) {
      			//		System.out.println("checked -> " + Integer.toString(key));
      					graph.blacklist_node("0." + Integer.toString(key));
      				}
      			}
      			for(Integer key : report_count.keySet()) {
      				if(report_count.get(key) == 0) {
      					graph.blacklist_node("0." + key);
      					 mote_checked.put(key,malicious_neighbor_table);
      				}
      				
      			}
      			
      		}
      	}, 60000, 60000);
  }

    private void startThreads() {
        Thread th = new Thread(new Listener(this));
        th.start();
        Thread th2 = new Thread(new Sender());
        th2.start();
//        sim = Cooja.cooja.getSimulation();
    
    }

    private void showController(){
        // set up GUI
        window = new JFrame("SD-IoT Controller");
        window.setSize(500, 200);
        textPane = new JTextPane();
        scrollPane = new JScrollPane(textPane);
        scrollPane.setViewportView(textPane);
        window.add(scrollPane);
        textField = new JTextField();
        
        window.add(BorderLayout.NORTH, textField);
        textField.addKeyListener(new KeyListener() {
        	
            @Override
            public void keyTyped(KeyEvent e) {

            }

            @Override
            public void keyPressed(KeyEvent e) {

            }

            @Override
            public void keyReleased(KeyEvent e) {
                /*Check if input text contains network size setting
                * e.g size=4 */
                if (e.getKeyCode() == KeyEvent.VK_ENTER) {
                    String inputTxt = textField.getText();
                    if (inputTxt.contains("size")){
                        String size = inputTxt.substring(inputTxt.indexOf("=")+1, inputTxt.length()).replaceAll("\\D+", "");
                        int netSize = Integer.parseInt(size);
                        networkSize = netSize;
                        textPane.setText("Network size is set to " + networkSize + "\n");
                        System.out.println("Network size is set to " + networkSize );
                    } else {
                        char[] openPathPacket = graph.getOpenPath(textField.getText());
                        sendToNode(openPathPacket);
                    }
                }
            }
        });
        window.setVisible(true);
    }

    private void printNodeInfo(JTextPane textPane) {
        StringBuilder b = new StringBuilder();
        textPane.setText(b.toString());
    }

    @Override
    public void update(Observable o, Object arg) {
        StringBuilder b = new StringBuilder();
        b.append(arg.toString() + "\n");
        graph.updateGraph(arg.toString().trim());
    }

    public void start(SerialConnection connection){
        if (hasStarted) {
            throw new IllegalStateException("already started");
        }
        hasStarted = true;
        this.serialConnection = connection;
        connectToSerial();
        showController();
        startThreads();
    }

    private void connectToSerial() {
        if (serialConnection != null && !serialConnection.isOpen()) {
            String comPort = serialConnection.getComPort();
            if (comPort == null && serialConnection.isMultiplePortsSupported()) {
                comPort = MoteFinder.selectComPort(window);
               
            }
            if (comPort != null || !serialConnection.isMultiplePortsSupported()) {
                serialConnection.open(comPort);
            }
        }
    }

    public boolean sendToNode(char[] data){
        if (serialConnection != null && serialConnection.isOpen() && serialConnection.isSerialOutputSupported()) {
            serialConsole.addSerialData("" + data);
            serialConnection.writeSerialData(data);
            return true;
        }
        return false;
    }

    @Override
    public void serialData(SerialConnection connection, String line) {
        inComingData = line;
    }

    @Override
    public void serialOpened(SerialConnection connection) {
        String connectionName = connection.getConnectionName();
        serialConsole.addSerialData("*** Serial console listening on " + connectionName + " ***");
        hasSerialOpened = true;
        if (connection.isMultiplePortsSupported()) {
            String comPort = connection.getComPort();
            configTable.put("collect.serialport", comPort);
        }
        System.out.println("connected to " + connectionName);

        if (!connection.isSerialOutputSupported()) {
            serialConsole.addSerialData("*** Serial output not supported ***");
        } else if (doSendInitAtStartup) {
            doSendInitAtStartup = false;
        }
    }

    @Override
    public void serialClosed(SerialConnection connection) {
        String prefix;
        if (hasSerialOpened) {
            serialConsole.addSerialData("*** Serial connection terminated ***");
            prefix = "Serial connection terminated.\n";
            hasSerialOpened = false;
            System.out.println("not connected");
        } else {
            prefix = "Failed to connect to " + connection.getConnectionName() + '\n';
        }
        if (!connection.isClosed()) {
            if (connection.isMultiplePortsSupported()) {
                String options[] = {"Retry", "Search for connected nodes", "Cancel"};
                int value = JOptionPane.showOptionDialog(window,
                        prefix + "Do you want to retry or search for connected nodes?",
                        "Reconnect to serial port?",
                        JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE,
                        null, options, options[0]);
                if (value == JOptionPane.CLOSED_OPTION || value == 2) {

                } else {
                    String comPort = connection.getComPort();
                    if (value == 1) {
                        comPort = MoteFinder.selectComPort(window);
                        if (comPort == null) {
                        }
                    }
                    if (comPort != null) {
                        connection.open(comPort);
                    }
                }
            }
        }
    }
    
    
    private class Listener extends Observable implements Runnable {

        public Listener(Controller controller){
            addObserver(controller);
        }
        public void check_neighbors(String[]Packet) {
        	int neighbor_mote;
        	for(int i = 11;i+1<Packet.length;i+=3) //display neighbor table
    		{ 
    			neighbor_mote = Integer.parseInt(Packet[i]);
    			if(neighbor_mote>50)
    				return;
        	
        	if(report_count.get(neighbor_mote)==null || report_count.get(neighbor_mote)==0) {
        		malicious_motes_newflow[neighbor_mote] = true;
				graph.blacklist_node("0."+Integer.toString(neighbor_mote));
				mote_checked.put(neighbor_mote,malicious_neighbor_table);
				}
    	    }
        }
    	public boolean check_mote(String inComingData) {
    	
    		
    		String[] Packet = inComingData.split(": ")[1].split(" ");
        	String x = "";
        	int current_mote =Integer.parseInt(Packet[3].split("[.]")[1]);
        	
  
        	if(malicious_motes_wormhole[current_mote])
        		return false;
      
        	
        	if(nf_detected) {
        		check_neighbors(Packet);
			for(int i = 11;i<Packet.length;i++)
				x = x+ Packet[i] + " ";
			if(x.equals(malicious_neighbor_table)) {
				System.out.println("Mote 0." + Integer.toString(current_mote) + " " + " blocked due to newflow");
				malicious_motes_newflow[current_mote] = true;
				graph.blacklist_node("0."+Integer.toString(current_mote));
				return false;
			}
    	}
        	if(malicious_motes_newflow[current_mote]) {
        	malicious_motes_newflow[current_mote] = false;
        	graph.go_online("0." + Integer.toString(current_mote));
        	}
          	
        	return true;
			
		}
        
        public void check_newflow(String inComingData) {
        	String[] Packet = inComingData.split(": ")[1].split(" ");
        	String x = "";
        	int current_mote =Integer.parseInt(Packet[3].split("[.]")[1]);
			for(int i = 11;i<Packet.length;i++)
				x = x+ Packet[i] + " ";
		
			if(mote_checked.get(current_mote) == null)
				mote_checked.put(current_mote,x);
			else
				return;
			
			if(neighbor_table_count.get(x)==null ||neighbor_table_count.get(x)==0)
				neighbor_table_count.put(x,1);
        	else
        		neighbor_table_count.put(x,neighbor_table_count.get(x)+1);
			
			if(neighbor_table_count.get(x) >=5) {
				System.out.println("NEW FLOW ATTACK DETECTED !!!!!! \n");
			    malicious_motes_newflow[current_mote] = true;
				graph.blacklist_node("0."+Integer.toString(current_mote));
				malicious_neighbor_table = x;
				sendToNode(("8" + " " + "1" + " " + malicious_neighbor_table).toCharArray());
				System.out.println("Malicious neighbor table -> " + malicious_neighbor_table + "\n");
				//check_newflow = false;
				nf_detected  = true;
				
			}
			
		}
       boolean check_range(double x1, double y1,double x2,double y2) {
		
    	   if( Math.sqrt(((x1-x2)*(x1-x2)) + ((y1-y2)*(y1-y2)))  >Transmission_range) 
    		   return true;
    	   else
    		   return false;
    	   
       }
       
       public int get_mote_from_packet(String data) {
		
    		String[] Packet = inComingData.split(": ")[1].split(" ");
    		return Integer.parseInt(Packet[3].split("[.]")[1]);
    	   
        	
        }
        
        public void check_wormhole(String data) {

        	String[] Packet = inComingData.split(": ")[1].split(" ");
           int current_mote =Integer.parseInt(Packet[3].split("[.]")[1]);
           int neighbor_mote;
    		
    		
    		for(int i = 11;i+1<Packet.length;i+=3) //display neighbor table
    		{ 
    			neighbor_mote = Integer.parseInt(Packet[i]);
    			if((malicious_motes_wormhole[neighbor_mote]&&malicious_motes_wormhole[current_mote]) || coordinates[current_mote][0] == 0.0 ||coordinates[neighbor_mote][0] == 0.0 )
    				continue;
  
    				//System.out.println("Checking motes:" +"0."+Integer.toString(current_mote) + "and " +"0."+Integer.toString(neighbor_mote) );
    			if(check_range(coordinates[current_mote][0],coordinates[current_mote][1],coordinates[neighbor_mote][0],coordinates[neighbor_mote][1])) {
    				System.out.println("WORMHOLE ATTACK DETECTED !!!!!! \n");
    				System.out.println("Malicious motes are:" +"0."+Integer.toString(current_mote) + "and " +"0."+Integer.toString(neighbor_mote) + "\n" );
    				graph.blacklist_node("0."+Packet[3].split("[.]")[1]);
    				graph.blacklist_node("0."+Packet[i]);
    				malicious_motes_wormhole[neighbor_mote] = true;
    				malicious_motes_wormhole[current_mote] = true;
    			}
    	    }
        	
        }
        
        public void get_coordinates() {
  
        	int mote_id;
        	 try
             { System.out.println("In function get_coordinates");
             File file = new File("/home/biohazard/sdn-wise-trickle/contiki/tools/sdnwise-controller/src/coordinates.txt"); 
             	RandomAccessFile raf = new RandomAccessFile(file, "rw"); 
             	 String[] data = raf.readLine().split(" "); 
             	 for(int i=0;i<data.length;i++) {
             		 mote_id = Integer.parseInt(data[i]);
             		
             		coordinates[mote_id][0] = Double.parseDouble(data[i+1]);
             		coordinates[mote_id][1] = Double.parseDouble(data[i+2]);
             		 i+=2;
             	}
             	 for(int i = 1 ;i<33;i++) {
             		 if(coordinates[i][0] == 0.0)
             			 continue;
             		 System.out.println("Mote " + Integer.toString(i)+ ": "+ "X =" + Double.toString(coordinates[i][0]) + "," + "Y =" + Double.toString(coordinates[i][1]));
             		 
             	 }
                  
                raf.close();
             } 
             catch (IOException fe) 
             { 
                 System.out.println("File not found"); 
             } 
       
        }
        @Override
        public void run() {
// 00:54.813	ID:1	Sink -> Controller: 1 22 0.1 0.2 2 100 0.1 1 58 3 0 1 79 0 3 70 0 8 78
//Sink -> Controller: 1 69 0.1 0.16 3 100 0.1 0 0 1 1 56 0 1 0 7 0 100 0 16
// 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45
// ID:6	[TXU]: 1 24 0.1 0.6 3 99 0.4 1 0 1 1 11 0 5 0 6 0 100 0 4 1
            String preData = null;
            String previousData = null;
            String prevmot = "";
            try {
                Thread.sleep(6000);
                
              while (inComingData != null) {
                    Thread.sleep(100);
                    if(!got_coordinates) {get_coordinates();got_coordinates = true;}
                    textPane.setText(inComingData);
                    if (!inComingData.equals(previousData)){
                        //System.out.println(inComingData);
                    }
                    if (inComingData.contains("Sink") && (!inComingData.equals(preData)) && check_mote(inComingData) ) {
                    	
                        String type = inComingData.split(": ")[1].split(" ")[4];
                        if(!type.equals("9"))
                    	System.out.println(inComingData);
                        
                        if(type.equals("2")) {
                        	
                        	if(check_wormholes)
                        	check_wormhole(inComingData);
                        	if(check_newflow)
                            check_newflow(inComingData);
                        	
                        	int x = get_mote_from_packet(inComingData);
                        	if(report_count.get(x)==null ||report_count.get(x)==0)
                        		report_count.put(x,1);
                        	else
                        		report_count.put(x,report_count.get(x)+1);
                        	
                            preData = inComingData;
                            setChanged();
                            notifyObservers(inComingData);
                        }
                        if(type.equals("3")) {
                            String responseDst = inComingData.split(": ")[1].split(" ")[3];
                            System.out.println("Resonpse packet Destination: " + responseDst);
                            sendToNode(graph.getOpenPath(responseDst));
                        }
                    
                 }
                    previousData = inComingData;
                }
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

        }

	
		
    }
    private class Sender implements Runnable {

        @Override
        public void run() {
            int numberOfNodes = 0;
            /*Scan until detect all nodes in the network*/
            while (true){
                try {
                    numberOfNodes = networkSize;
                    Thread.sleep(1000);
                    if (numberOfNodes != 0 && graph.getAllNodes().size() == numberOfNodes){
                        break;
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            /*Send Open Path packets to all nodes, or send to specific node via GUI*/
            for (String node : graph.getAllNodes()){
                char[] openPath = graph.getOpenPath(node);
                sendToNode(openPath);
                try {
                    Thread.sleep(200);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}