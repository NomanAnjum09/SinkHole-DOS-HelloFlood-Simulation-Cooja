# SDN-WISE Controller 
## Code structure

contiki

|---tools

    |---cooja
        |---java
            |---org.contikios.cooja
                |---plugins
                    |---SDNWISE.java
                |---Cooja.java    
    |---collect-view
    |---sdnwise-controller
        |---src
            |---controller
                |---Controller.java
                |---Loader.java
                |---NetworkGraph

where    
`SDNWISE.java`: plugin connecting Sink node with SDN-WISE
Controller in Cooja.

`Cooja.java`(modified): add SDNWISE class as a plugin

  `public void reparseProjectConfig() throws ParseProjectsException 
  { 
    /*...*/
    //TODO add SDNWISE plugin
        Class<? extends Plugin> sdnWise = tryLoadClass(this, Plugin.class,
                "org.contikios.cooja.plugins.SDNWISE");
    /*...*/    
  }`

`collect-view`: all interfaces for connection between Contiki
node and external applications. 

`NetworkGraph.java`: represent network attributes (nodes, edges).

`Controller.java`: response Request Open Path packet from nodes or send
Open Path packet automatically/manually.

`Loader.java`: start Controller with approriate connection type. 

## Usage
**Build collect-view connection**

cd tools/collect-view

ant run

**Build cooja simulation**

cd tools/cooja

ant run

**Start controller plugin**

In the Cooja simulation window:

Tools -> SDN-WISE -> Start controller


**Scenario**

Set the network size variable into the Controller by Enter 

`size=4` (e.g. topology consists of 4 nodes) in the WISE Controller 
window.
After detecting all nodes, Controller will send Open Path packets to 
all nodes.

If size variable is not set, Controller will response Request packets 
to nodes which have no matching rule.

Open Path packet can be sent to a specific node by enter node-id 
in the WISE Controller window

`e.g. send Open Path to node 0.4 -> 0.4 Enter`

**SDN-WISE Controller with z1 mote**

Connect Sink node to PC, connected port: `ttyUSBx`

Copy `sdnwise-controller/build/jar/sdnwise-controller.jar` to `tools/collect-view/dist`

Run controller int directory tools/collect-view/dist

`java -jar sdnwise-controller.jar /dev/ttyUSBx`













