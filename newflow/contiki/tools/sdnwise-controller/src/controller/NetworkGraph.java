package controller;

import java.util.ArrayList;
import java.util.Currency;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.Vector;
import java.util.concurrent.*;

import javax.security.auth.Refreshable;

import org.apache.commons.lang3.ObjectUtils.Null;
import org.graphstream.algorithm.*;
import org.graphstream.graph.*;
import org.graphstream.graph.implementations.*;

/**
 * Created by ABD.
 */
public class NetworkGraph {
	Graph graph;
	Dijkstra dijkstra;
	protected String sheetstyle = "node {\n" + "\tsize: 15px;\n" + "\tfill-color: black;\n" + "\tstroke-mode: plain;\n"
			+ "\tstroke-color: yellow;\n" + "\tshadow-mode: gradient-vertical;\n" + "\tshadow-offset: 2;\n" + "}\n"
			+ "edge {" + "size: 2px;" + "}" + "\n" + "node#0.1 {\n" + "\tfill-color: #3333cc;\n" + "}\n" + "\n"
			+ "node:clicked {\n" + "\tfill-color: red;\n" + "}";

	int onlinemotes = 0;
	int offlinemotes = 0;
	int tval = 5;
	int blacklistedcount = 0;
	int whitelistedcount = 0;
	int releaseOpenPath = 0;
	int highestThresholdValue = 0;
	String highestThresholdMoteId = "";
	int transferControlToCurTable = 0;
	
	
	Vector<String> blacklist = new Vector<String>();
	ConcurrentHashMap<String, String> refTable = new ConcurrentHashMap<String, String>();
	ConcurrentHashMap<String, String> curTable = new ConcurrentHashMap<String, String>();
	ConcurrentHashMap<String, Integer> threshold = new ConcurrentHashMap<String, Integer>();
	ConcurrentHashMap<String, String> neighborTable = new ConcurrentHashMap<String, String>();
	Vector<String> sendOpenPathTable = new Vector<String>();

	public NetworkGraph() {

		graph = new SingleGraph("SDN WISE Network");
		graph.setStrict(false);
		graph.setAutoCreate(true);
		graph.addAttribute("ui.stylesheet", sheetstyle);
		graph.display();
		dijkstra = new Dijkstra(Dijkstra.Element.EDGE, null, "length");

		Timer timer010 = new Timer();
		timer010.scheduleAtFixedRate(new TimerTask() {
			@Override
			public void run() {
				
				onlinemotes = 0;
				offlinemotes = 0;
				transferControlToCurTable += 1;
				
				curTable.forEach((key, val) -> {
					refTable.put(key, "online");
					System.out.println("Online Motes ID: " + key);
					onlinemotes += 1;
				});

				refTable.forEach((key, val) -> {
					if (!curTable.containsKey(key)) {
						if (graph.getNode(key) != null) {
							//graph.removeNode(key);
						}
						refTable.put(key, "offline");
						System.out.println("Offline Motes ID: " + key);
						offlinemotes += 1;
					}
				});
				curTable.clear();
				System.out.println("Online Motes Count: " + onlinemotes);
				System.out.println("Offline Motes Count: " + offlinemotes);
			}
		}, 120000, 120000);

		Timer timer020 = new Timer();
		timer020.scheduleAtFixedRate(new TimerTask() {
			@Override
			public void run() {
				releaseOpenPath += 1;
				threshold.forEach((key, val) -> {
					if (val > highestThresholdValue) {
						highestThresholdValue = val;
						highestThresholdMoteId = key;
					}
					System.out.println("Network Threshold -> (Mote ID: " + key + ", Threshold Value: " + val + ")");
					if (val > 12) {
						if (!blacklist.contains(key)) {
							//blacklist.add(key);
							System.out.println("Blacklisted Mote -> (ID: " + key + ", Value: " + val + ")");
						}
					}
				});
				threshold.clear();
				whitelistedcount = 0;
				blacklistedcount = 0;
				
				System.out.println("************ All Blacklisted Motes ************");
				Iterator<String> it = blacklist.iterator();
				while(it.hasNext()) {
					System.out.println( "Blacklisted Mote ID: " + it.next() );
					blacklistedcount++;
				}
				System.out.println("************ All Whitelisted Motes ************");
				refTable.forEach((key, val) -> {
					if(val.equals("online")) {
						System.out.println( "Whitelisted Mote ID: " + key );
						whitelistedcount++;
					}
				});
				System.out.println("Network Highest Threshold Value: " + highestThresholdValue + " Mote ID: " + highestThresholdMoteId);
				System.out.println("Blacklisted Count: " + blacklistedcount);
				System.out.println("Whitelisted Count: " + whitelistedcount);
				highestThresholdValue = 0;
				highestThresholdMoteId = "";
				
			}
		}, 60000, 60000);
		
		Timer timer030 = new Timer();
		timer030.scheduleAtFixedRate(new TimerTask() {

			@Override
			public void run() {
				// TODO Auto-generated method stub
				
				if (blacklist.size() > 0) {
					String mmdpacket = "9 1 ";
					for (String value : blacklist) {
						
						mmdpacket = mmdpacket + value.substring(2) + " ";
						refTable.put(value, "offline");
					}
					System.out.println("[timer030] MMD: "+mmdpacket);
					Loader.controller.sendToNode(mmdpacket.toCharArray());
				}
				buildSPT();
			}
		}, 70000, 30000);
	}

	public void updateGraph(String inComingData) {

		String reportPacket = inComingData.substring(inComingData.indexOf(":") + 2);
		String packet[] = reportPacket.split(" ");
		String SINK = packet[2];
		String SENDER = packet[3];
		String HOPS = packet[7];
		LinkedList<String> NEIGHBORS = new LinkedList<>();

		if (blacklist.contains(SENDER)) {
			if(refTable.containsKey(SENDER)) {
				refTable.remove(SENDER);	
			}
			if (graph.getNode(SENDER) != null) {
				//graph.removeNode(SENDER);
			}
			if(curTable.containsKey(SENDER)) {
				curTable.remove(SENDER);
			}
			return;
		}
		
		curTable.put("0.1", "online");
		curTable.put(SENDER, "online");
		
		if(!SENDER.contentEquals("0.1")) {
			int thresholdvalue = threshold.getOrDefault(SENDER, 1) + 1;
			if(getThresh(packet[1]) > 0) {
				threshold.put(SENDER, (getCurrentThreshold() + thresholdvalue) );
				return;
			}
			threshold.put(SENDER, thresholdvalue);
		}
		
		 if(!packet[4].equals("8"))
		for (int i = 1; i <= Integer.parseInt(packet[9]); i++) {
			NEIGHBORS.add(packet[9 + 3 * i - 2] + "." + packet[9 + 3 * i - 1] + "\t" + packet[9 + 3 * i]);
		}

		if (graph.getNode(SINK) == null) {
			graph.addNode(SINK);
		}
		if (graph.getNode(SENDER) == null) {
			graph.addNode(SENDER);
		}
		if (graph.getNode(SENDER) != null) {
			graph.getNode(SENDER).addAttribute("neighbors", NEIGHBORS);
		}

		int deletemote = 0;
		System.out.println("Neighbor Table of Mote ID: " + SENDER);
		for (String tmp : NEIGHBORS) {
			String neighbor = tmp.split("\t")[0];
			String rssi = tmp.split("\t")[1];
			if(blacklist.contains(neighbor)) {
				continue;
			}
			String ofl = refTable.getOrDefault(neighbor, "");  
			if(ofl.contentEquals("offline")) {
				continue;
			}
			System.out.println("Neighbor Table of " + SENDER + " -> (Neighbor Mote: " + neighbor + " Neighbor Mote's RSSI Value:" + rssi + ")");
			if (neighbor.contentEquals("0.1")) {
				String previousRSSI = neighborTable.getOrDefault(SENDER, "0");
				if ((!previousRSSI.contentEquals(rssi)) && (!previousRSSI.contentEquals("0"))) {
					deletemote = 1;
				}
				neighborTable.put(SENDER, rssi);
			}
			graph.addNode(neighbor);
			graph.addEdge(SENDER + "-" + neighbor, SENDER, neighbor);
			if (graph.getEdge(SENDER + "-" + neighbor) != null) {
				graph.getEdge(SENDER + "-" + neighbor).addAttribute("length", Integer.parseInt(rssi));
			}
		}

		for (org.graphstream.graph.Node n : graph) {
			n.addAttribute("ui.label", n.getId());
		}

		for (Edge e : graph.getEachEdge()) {
			e.addAttribute("label", (int) e.getNumber("length"));
		}
		
		if ( (releaseOpenPath > 0) && !(sendOpenPathTable.contains(SENDER)) && (refTable.containsKey(SENDER)) ) {
			Loader.controller.sendToNode(getOpenPath(SENDER));
			sendOpenPathTable.add(SENDER);
		}

		if (deletemote == 1) {
			if (graph.getNode(SENDER) != null) {
				//graph.removeNode(SENDER);
			}
			if(sendOpenPathTable.contains(SENDER)) {
				sendOpenPathTable.remove(SENDER);
			}
			if(refTable.containsKey(SENDER)) {
				refTable.remove(SENDER);
			}
			if(curTable.containsKey(SENDER)) {
				curTable.remove(SENDER);
			}	
		}
	}

	private synchronized void buildSPT() {
		dijkstra.init(graph);
		dijkstra.setSource(graph.getNode("0.1"));
		dijkstra.compute();
		for (Edge e : graph.getEachEdge()) {
			e.addAttribute("ui.style", "fill-color: #ffffcc;");
		}
		for (Edge edge : dijkstra.getTreeEdges()) {
			edge.addAttribute("ui.style", "fill-color: red;");
		}
	}

	int getThresh(String mote) {
		if(mote.toCharArray()[0]=='1') {
			if(mote.toCharArray()[1]=='4') {
				return 1;
			}	
		}
		return 0;
	}
	
	public char[] getOpenPath(String destination) {
		buildSPT();
		String dest = destination;
		List<Node> path = new ArrayList<>();
		if (graph.getNode(dest) != null) {
			for (Node node : dijkstra.getPathNodes(graph.getNode(dest))) {
				path.add(0, node);
			}
		}

		System.out.print("{");
		for (Node n : path) {
			System.out.print(n.getId() + ", ");
		}
		System.out.print("}" + "\n");

		int packetLength = 11 + path.size() * 2;
		ArrayList<String> packetString = new ArrayList<>();
		// add header 1, 19, 0, 1, 0, 2, 5, 100, 0, 1, 0, 0, 1, 0, 2, 0, 3, 0, 4,
		packetString.add("1");
		packetString.add(Integer.toString(packetLength));
		packetString.add("0");
		packetString.add("1");
		packetString.add("0");
		packetString.add("1");
		packetString.add("5");
		packetString.add("90");
		packetString.add("0");
		packetString.add("1");
		packetString.add("0");
		// add path
		for (Node node : path) {
			packetString.add(node.getId().split("\\.")[0]);
			packetString.add(node.getId().split("\\.")[1]);
			node.getEnteringEdgeIterator();
		}

		char[] openPath = new char[packetLength];
		for (int i = 0; i < packetLength; i++) {
			openPath[i] = (char) (Integer.parseInt(packetString.get(i)) + 32);
			// openPath[i] = (char) (Integer.parseInt(packetString.get(i)));
		}
		// print to test openPath
		System.out.print("OpenPath :{");
		for (int i = 0; i < openPath.length; i++) {
			System.out.print((int) openPath[i] + " ");
		}
		System.out.print(" }" + "\n");
		return openPath;
	}
	
	int getCurrentThreshold(){
		return tval;
	}
	
	public String getOpenPathString(String destination) {
		buildSPT();
		String dest = destination;
		List<Node> path = new ArrayList<>();
		if (graph.getNode(dest) != null) {
			for (Node node : dijkstra.getPathNodes(graph.getNode(dest))) {
				path.add(0, node);
			}
		}

		int packetLength = 11 + path.size() * 2;
		ArrayList<String> packetString = new ArrayList<>();
		// add header
		packetString.add("1");
		packetString.add(Integer.toString(packetLength));
		packetString.add("0");
		packetString.add("1");
		packetString.add("0");
		packetString.add("1");
		packetString.add("5");
		packetString.add("90");
		packetString.add("0");
		packetString.add("1");
		packetString.add("0");
		// add path
		for (Node node : path) {
			packetString.add(node.getId().split("\\.")[0]);
			packetString.add(node.getId().split("\\.")[1]);
			// node.getEnteringEdgeIterator();
		}

		StringBuilder openPath = new StringBuilder();
		for (String s : packetString) {
			openPath.append(s + " ");
		}
		// print to test openPath
		System.out.print("OpenPath :{");
		System.out.print(openPath);
		System.out.print(" }" + "\n");
		return String.valueOf(openPath);
	}
	
	

	public ArrayList<String> getLeafNodes() {
		ArrayList<String> leafNodes = new ArrayList<>();
		buildSPT();
		for (Node node : graph.getEachNode()) {
			if (!node.getId().equals("0.1")) {
				ArrayList<Node> path = new ArrayList<>();
				for (Node n : dijkstra.getPathNodes(node)) {
					path.add(0, n);
				}
				for (int i = 0; i < path.size() - 1; i++) {
					path.get(i).addAttribute("children", path.get(i + 1).getId());
				}
			}
		}
		for (Node node : graph.getEachNode()) {
			if (node.getAttribute("children") == null) {
				node.addAttribute("isLeaf", true);
				node.addAttribute("label", "leaf");
				leafNodes.add(node.getId());
			}
		}
		return leafNodes;
	}

	public ArrayList<String> getAllNodes() {
		ArrayList<String> listNodes = new ArrayList<>();
		for (Node n : graph.getEachNode()) {
			listNodes.add(n.getId());
		}
		return listNodes;
	}
	 public void remove(ArrayList<String> motes) {
		    for(int i=0 ;i<motes.size();i++) {
		    	
		    	if (graph.getNode(motes.get(i)) != null) {
		    		graph.removeNode(motes.get(i));
		    	}
		        
		    	if (!blacklist.contains(motes.get(i))) {
		    		//blacklist.add(motes.get(i));
		    		String mmdpacket = "MMD: ";
						for (String value : blacklist) {
							
							
							mmdpacket = mmdpacket + value + " ";
							refTable.put(value, "offline");
						}
						Loader.controller.sendToNode(mmdpacket.toCharArray());	
		    	}
		    }
		}
	 public void blacklist_node(String mote) {
		 if (!blacklist.contains(mote)) {
	    		blacklist.add(mote);
	    		String mmdpacket = "9 1 ";
				for (String value : blacklist) {
					
					mmdpacket = mmdpacket + value.substring(2) + " ";
					refTable.put(value, "offline");
				}
					Loader.controller.sendToNode(mmdpacket.toCharArray());	
		 
		  
	  }
		 if (graph.getNode(mote) != null) {
	    		graph.removeNode(mote);
		 }
		 
}
	 public void go_online(String mote) {
		 System.out.println("GO ONLINE --->" + mote);
		 blacklist.remove(mote);
		 refTable.put(mote,"online");
	 }
	 
	 

}