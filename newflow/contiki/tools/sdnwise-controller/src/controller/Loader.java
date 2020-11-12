package controller;

import org.contikios.contiki.collect.*;

/**
 * Created by ABD.
 */
public class Loader {
	
	public static Controller controller = new Controller();
    
	public static void main(String args[]){
        String STDIN_COMMAND = "<STDIN>";
        boolean resetSensorLog = false;
        boolean useSensorLog = true;
        boolean useSerialOutput = true;
        String host = null;
        String command = null;
        String logFileToLoad = null;
        String comPort = null;
        int port = -1;
        for(int i = 0, n = args.length; i < n; i++) {
            String arg = args[i];
            if (arg.length() == 2 && arg.charAt(0) == '-') {
                switch (arg.charAt(1)) {
                    case 'a':
                        if (i + 1 < n) {
                            host = args[++i];
                            int pIndex = host.indexOf(':');
                            if (pIndex > 0) {
                                port = Integer.parseInt(host.substring(pIndex + 1));
                                host = host.substring(0, pIndex);
                            }
                        } else {
                            usage(arg);
                        }
                        break;
                    case 'c':
                        if (i + 1 < n) {
                            command = args[++i];
                        } else {
                            usage(arg);
                        }
                        break;
                    case 'p':
                        if (i + 1 < n) {
                            port = Integer.parseInt(args[++i]);
                        } else {
                            usage(arg);
                        }
                        break;
                    case 'r':
                        resetSensorLog = true;
                        break;
                    case 'n':
                        useSensorLog = false;
                        break;
                    case 'i':
                        useSerialOutput = false;
                        break;
                    case 'f':
                        command = STDIN_COMMAND;
                        if (i + 1 < n && !args[i + 1].startsWith("-")) {
                            logFileToLoad = args[++i];
                        }
                        break;
                    case 'h':
                        usage(null);
                        break;
                    default:
                        usage(arg);
                        break;

                }
            } else if (comPort == null){
                comPort = arg; /*comport = /dev/ttyUSBx*/
            } else {
                usage(arg);
            }
        }
        
        SerialConnection serialConnection;
        /*choose connection type*/
        if (host != null) {
            if (port <= 0) {
                port = 60001;
            }
            // TODO modify TCP connection constructor to match Controller
            serialConnection = new TCPClientConnection(controller, host, port);
        } else if (port > 0) {
            //  modify UDP connection constructor
            serialConnection = new UDPConnection(controller, port);
        } else if (command == null) { /*this is our case*/
            //  modify SerialDumpConnection constructor
            serialConnection = new SerialDumpConnection(controller);
        } else if (command == STDIN_COMMAND) {
            serialConnection = new StdinConnection(controller);
        } else {
            //  modify CommandConnection constructor
            serialConnection = new CommandConnection(controller, command);
        }
        if (comPort == null) {
            //comPort = server.getConfig("collect.serialport");
        }
        if (comPort != null) {
            serialConnection.setComPort(comPort);
        }
        if (!useSerialOutput) {
            serialConnection.setSerialOutputSupported(false);
        }
        //SerialConnection serialConnection = new StdinConnection(controller); /*comPort = null*/
        controller.start(serialConnection);
        System.out.println("host: " + host);
        System.out.println("port: " + port);
        System.out.println("command: "+ command);
        System.out.println("comPort: " + comPort);
        System.out.println("useSerialOutput: " + useSerialOutput);
        System.out.println("logFileToLoad: " + logFileToLoad);
        System.out.println("resetSensorLog: " + resetSensorLog);
    }

    private static void usage(String arg) {
        if (arg != null) {
            System.err.println("Unknown argument '" + arg + '\'');
        }
        System.err.println("Usage: java CollectServer [-n] [-i] [-r] [-f [file]] [-a host:port] [-p port] [-c command] [COMPORT]");
        System.err.println("       -n : Do not read or save sensor data log");
        System.err.println("       -r : Clear any existing sensor data log at startup");
        System.err.println("       -i : Do not allow serial output");
        System.err.println("       -f : Read serial data from standard in");
        System.err.println("       -a : Connect to specified host:port");
        System.err.println("       -p : Read data from specified UDP port");
        System.err.println("       -c : Use specified command for serial data input/output");
        System.err.println("   COMPORT: The serial port to connect to");
        System.exit(arg != null ? 1 : 0);
    }
}
