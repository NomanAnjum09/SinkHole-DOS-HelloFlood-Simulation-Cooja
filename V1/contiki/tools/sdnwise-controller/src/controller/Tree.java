package controller;

public class Tree {
	Node root;

	// Tree Node
	static class Node {
		int data;
		Node left, right;

		Node(int data) {
			this.data = data;
			this.left = null;
			this.right = null;
		}
	}

	// Function to insert nodes in level order
	public Node insertLevelOrder(int[] arr, Node root, int i) {
		// Base case for recursion
		if (i < arr.length) {
			Node temp = new Node(arr[i]);
			root = temp;

			// insert left child
			root.left = insertLevelOrder(arr, root.left, 2 * i + 1);

			// insert right child
			root.right = insertLevelOrder(arr, root.right, 2 * i + 2);
		}
		return root;
	}

	// Function to print tree nodes in InOrder fashion
	public void inOrder(Node root) {
		if (root != null) {
			inOrder(root.left);
			System.out.print(root.data + " ");
			inOrder(root.right);
		}
	}

	/*
	 * add this code later after checking it seperately...
	 * 
	 * static Node findParentWithHighestLevel(Node node) { int inDegreeOfNode =
	 * node.getInDegree(); Node parent = null;
	 * 
	 * Iterator<Edge> nodeIterator = node.getEachEnteringEdge().iterator();
	 * if(inDegreeOfNode == 1) parent = nodeIterator.next().getOpposite(node); else
	 * if (inDegreeOfNode > 1) { parent = nodeIterator.next().getOpposite(node);
	 * while (nodeIterator.hasNext()) { Node temp =
	 * nodeIterator.next().getOpposite(node); if (temp.hasAttribute("layoutLayer")
	 * && (int) temp.getAttribute("layoutLayer") > (int)
	 * parent.getAttribute("layoutLayer")) { parent = temp; } }
	 * 
	 * }
	 * 
	 * if(parent != null && !parent.hasAttribute("layouted")) {
	 * parent.setAttribute("layouted", "true"); positionNode(parent); } return
	 * parent; }
	 * 
	 * private void unlabelAdjacentEdges(Node n) { for (Edge e : n.getEdgeSet()){
	 * Node v1 = e.getNode0(); Node v2 = e.getNode1();
	 * 
	 * String ui_label_v1 = v1.getAttribute("ui.label"); String ui_label_v2 =
	 * v2.getAttribute("ui.label");
	 * 
	 * 
	 * if ((ui_label_v1==null || ui_label_v1.equals("")) && (ui_label_v2==null ||
	 * ui_label_v2.equals(""))) { e.setAttribute("ui.label", ""); } }
	 * 
	 * }
	 */

}
