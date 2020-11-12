package org.contikios.cooja.plugins;

import org.apache.log4j.Logger;
import org.contikios.cooja.*;

import javax.swing.*;
import java.awt.*;
import java.awt.EventQueue;
import java.awt.event.*;
import java.io.*;
import java.util.LinkedList;
import java.util.Observable;
import java.util.Observer;

import org.contikios.cooja.ClassDescription;
import org.contikios.cooja.PluginType;
import org.contikios.cooja.interfaces.SerialPort;

/**
 * Created by Hieu Nguyen on 12/16/2017.
 */
@ClassDescription("SDN WISE")
@PluginType(PluginType.SIM_PLUGIN)

public class SDNWISE extends VisPlugin{
    private JFrame frame;
    private Simulation sim;
    private JTextPane textPane;
    private JButton button;
    private LogListener myLogListener;
    private static final long serialVersionUID = 4368807123350830772L;
    private Mote sdnSink;
    // private CollectServer collectServer;

    //Test CollecView app
    private SerialPort serialPort;
    private static Logger logger = Logger.getLogger(SDNWISE.class);
    private JLabel inLabel, outLabel;
    private Observer serialDataObserver;
    private boolean isRunning;
    private Process commandProcess;
    private DataOutputStream out;
    private int inBytes = 0, outBytes = 0;
    JScrollPane scrollPane;

    String jarPath = "/tools/sdnwise-controller/build/jar/sdnwise-controller.jar";

    public SDNWISE(Simulation simulation, Cooja gui) {
        super("SDNWISE", gui);
        this.sim = simulation;
        textPane = new JTextPane();
        sdnSink = sim.getMoteWithID(1);
        //collectServer = new CollectServer();
        scrollPane = new JScrollPane(textPane);

        button = new JButton("Start controller");
        button.addMouseListener(new MouseListener() {
            @Override
            public void mouseClicked(MouseEvent e) {
                Plugin myPlugin = sim.getCooja().getPlugin("LogListener" );
                //printElements(myPlugin);
                myLogListener = (LogListener) myPlugin;
                if (e.getButton() == MouseEvent.BUTTON1) {
                    //printMoteInfo(sdnSink);
                    startController();
                }
                if (e.getButton() == MouseEvent.BUTTON3) {
                    printData(myLogListener);
                }
            }

            @Override
            public void mousePressed(MouseEvent e) {

            }

            @Override
            public void mouseReleased(MouseEvent e) {

            }

            @Override
            public void mouseEntered(MouseEvent e) {

            }

            @Override
            public void mouseExited(MouseEvent e) {

            }
        });
        add(BorderLayout.NORTH, button);
    }

    private void startController() {

        /* Mote serial port */
        serialPort = (SerialPort) sdnSink.getInterfaces().getLog();
        if (serialPort == null) {
            throw new RuntimeException("No mote serial port");
        }

          /* GUI components */
        if (Cooja.isVisualized()) {
            inLabel =  new JLabel("      0 bytes", JLabel.RIGHT);
            outLabel = new JLabel("      0 bytes", JLabel.RIGHT);

            JPanel panel = new JPanel(new GridBagLayout());
            GridBagConstraints c = new GridBagConstraints();
            c.fill = GridBagConstraints.HORIZONTAL;
            c.insets.left = c.insets.right = c.insets.top = 6;
            c.gridy = 0;

            panel.add(new JLabel("Controller -> Sink:"), c);
            panel.add(inLabel, c);

            c.insets.bottom = 6;
            c.gridy++;
            panel.add(new JLabel("Sink -> Controller:"), c);
            panel.add(outLabel, c);

            getContentPane().add(BorderLayout.NORTH, panel);
            pack();
        }

        String contikiPath = Cooja.getExternalToolsSetting("PATH_CONTIKI", "../../..");
        String jarFile = contikiPath + jarPath ;

        if (!new File(jarFile).canRead()) {
            logger.fatal("Could not find the CollectView application:" + jarFile);
            if (Cooja.isVisualized()) {
                JOptionPane.showMessageDialog(Cooja.getTopParentContainer(),
                        "Could not find the SDNWISE controller application:\n" +
                                jarFile + "\n\nPlease try to recompile it!",
                        "SDNWISE application not found", JOptionPane.ERROR_MESSAGE);
            }
            cleanup();
            return;
        }


        try {
            String[] cmd = new String[] {
                    "java", "-jar", jarFile,
                    "-n", "-f"
            };

            isRunning = true;
            commandProcess = Runtime.getRuntime().exec(cmd);
            final BufferedReader input = new BufferedReader(new InputStreamReader(commandProcess.getInputStream()));
            final BufferedReader err = new BufferedReader(new InputStreamReader(commandProcess.getErrorStream()));
            out = new DataOutputStream(commandProcess.getOutputStream());

      /* Start thread listening on standard out */
            Thread readInput = new Thread(new Runnable() {
                public void run() {
                    String line;
                    try {
                        while ((line = input.readLine()) != null) {
                            if (line.length() > 0) {
                                System.err.println("Controller Serial> " + line);
                                for (int i = 0, n = line.length(); i < n; i++) {
                                    serialPort.writeByte((byte) line.charAt(i));
                                }
                                serialPort.writeByte((byte) '\n');
                                inBytes += line.length() + 1;
                                if (Cooja.isVisualized()) {
                                    inLabel.setText(inBytes + " bytes");
                                }
                            }
                        }
                        input.close();
                    } catch (IOException e) {
                        if (isRunning) {
                            logger.error("The SDNWISE controller application died!", e);
                        }
                    } finally {
                        cleanup();
                    }
                }
            }, "read input stream thread");

      /* Start thread listening on standard err */
            Thread readError = new Thread(new Runnable() {
                public void run() {
                    String line;
                    try {
                        while ((line = err.readLine()) != null) {
                            System.err.println("Controller> " + line);
                        }
                        err.close();
                    } catch (IOException e) {
                        if (isRunning) {
                            logger.error("The SDNWISE controller application died!", e);
                        }
                    }
                }
            }, "read error stream thread");

            readInput.start();
            readError.start();
        } catch (Exception e) {
            throw (RuntimeException) new RuntimeException(
                    "CollectView error: " + e.getMessage()).initCause(e);
        }

        /* Observe serial port for outgoing data */
        serialPort.addSerialDataObserver(serialDataObserver = new Observer() {
            public void update(Observable obs, Object obj) {
                DataOutputStream out = SDNWISE.this.out;
                if (out != null) {
                    try {
                        byte b = serialPort.getLastSerialData();
                        out.write(b);
                        /*test */
                        //System.out.println(out);
                        outBytes++;
                        if (b == '\n') {
                            out.flush();
                            /*test serial output*/
                            //System.err.println(out);
                            if (Cooja.isVisualized()) {
                                outLabel.setText(outBytes + " bytes");
                            }
                        }
                    } catch (IOException e) {
                        if (isRunning) {
                            logger.warn("SDNWISE client output error", e);
                        }
                    }
                }
            }
        });
    }

    private void printMoteInfo(Mote sdnSink) {
        StringBuilder b = new StringBuilder();
        LinkedList<String> moteInfo = new LinkedList<String>();
        moteInfo.add("Sink ID: " + sdnSink.getInterfaces().getMoteID().toString()); // getMote ID
        moteInfo.add("Sink RIME: "+ sdnSink.getInterfaces().getRimeAddress().getAddressString()); // get RIME
        moteInfo.add("Sink log: " + sdnSink.getInterfaces().getLog().getLastLogMessage()); // get LastLog msg
        moteInfo.add("Sink source file: " + sdnSink.getType().getContikiSourceFile()); // get source File
        for (String tmp : moteInfo) {
            b.append(tmp + "\n");
        }
        b.append("Test moteInterfaceHanlder!");
        textPane.setText(b.toString());
        add(BorderLayout.SOUTH, textPane);
    }

    private void printData(LogListener myLogListener) {
        StringBuilder b = new StringBuilder();
        String tmp;
        for (int i = 0; i < myLogListener.getModel().getRowCount(); i++) {
            tmp = myLogListener.getModel().getValueAt(i, 2).toString();
            if (tmp.contains("Sink"))
                // b. append(myLogListener.getModel().getValueAt(i, 2).toString() + "\n");
                b.append(tmp + "\n");
        }
        textPane.setText(b.toString());
        scrollPane.setViewportView(textPane);
        add(BorderLayout.SOUTH, scrollPane);

    }

    private void printElements(Plugin plugin) {

        textPane.setText(plugin.toString());
        add(BorderLayout.SOUTH, textPane);
    }

    private void cleanup() {
        if (serialDataObserver != null) {
            serialPort.deleteSerialDataObserver(serialDataObserver);
            serialDataObserver = null;
        }

        if (isRunning) {
            logger.fatal("The SDNWISE application died!");
            if (Cooja.isVisualized()) {
                JOptionPane.showMessageDialog(this, "The SDNWISE client application died!",
                        "SDNWISE died!", JOptionPane.ERROR_MESSAGE);
            }
        }
        isRunning = false;
        if (commandProcess != null) {
            commandProcess.destroy();
            commandProcess = null;
        }
        if (out != null) {
            try {
                out.close();
                out = null;
            } catch (IOException e) {
            }
        }

        if (Cooja.isVisualized()) {
            EventQueue.invokeLater(new Runnable() {
                public void run() {
                    setTitle(getTitle() + " *DISCONNECTED*");
                    inLabel.setEnabled(false);
                    outLabel.setEnabled(false);
                }
            });
        }
    }

}