package controller;

import connection.SerialConsole;
import org.contikios.contiki.collect.MoteFinder;
import org.contikios.contiki.collect.SerialConnection;
import org.contikios.contiki.collect.SerialConnectionListener;

import javax.swing.*;
import java.awt.*;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.util.Observable;
import java.util.Observer;
import java.util.Properties;

public class Controller implements Observer, SerialConnectionListener {
	
    String inComingData;
    JFrame window;
    JTextPane textPane;
    JScrollPane scrollPane;
    JTextField textField;
    JButton button;
    NetworkGraph graph;
    private SerialConsole serialConsole;
    private SerialConnection serialConnection;
    private static final String MAIN = "main";
    private boolean doSendInitAtStartup = false;
    private Properties configTable = new Properties();
    private boolean hasSerialOpened;
    private boolean hasStarted = false;
    private int networkSize;

    public Controller(){
        serialConsole = new SerialConsole(this, MAIN);
        graph = new NetworkGraph();
    }

    private void startThreads() {
        Thread th = new Thread(new Listener(this));
        th.start();
        Thread th2 = new Thread(new Sender());
        th2.start();
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
                    textPane.setText(inComingData);
                    if (!inComingData.equals(previousData)){
                        //System.out.println(inComingData);
                    }
                    if (inComingData.contains("Sink") && (!inComingData.equals(preData))) {
                        System.out.println(inComingData);
                        String type = inComingData.split(": ")[1].split(" ")[4];
                        if(type.equals("2")) {
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
