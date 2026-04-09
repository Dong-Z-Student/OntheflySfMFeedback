#pragma once
#include "graph.h"
#include <iostream>
#include <map>
#include <vector>

class DynamicGraphCut {
public:
	typedef Graph<double, double, double> GraphType;

private:
	GraphType* graph_;
	int num_nodes_;

	struct EdgeInfo {
		GraphType::arc_id forward_arc;
		GraphType::arc_id reverse_arc;
		int from, to;
	};

	std::map<std::pair<int, int>, EdgeInfo> edge_map_;
	bool first_maxflow_;

public:
	DynamicGraphCut(int num_nodes, int estimated_edges = 0);
	~DynamicGraphCut();

	void addEdge(int i, int j, double cap, double rev_cap = 0.0);
	void setTerminalWeights(int i, double source_cap, double sink_cap);

	void removeNode(int i);
	void updateEdgeCapacity(int i, int j, double new_cap, double new_rev_cap);
	void updateTerminalWeights(int i, double source_cap, double sink_cap);
	void batchUpdateEdges(const std::vector<std::tuple<int, int, double, double>>& updates);

	double computeMaxFlow(bool reuse_trees = true, Block<int>* changed_list = nullptr);
	int getSegment(int i);
	std::vector<std::pair<int, int>> getCutEdges();
	std::vector<int> getChangedNodes(Block<int>* changed_list);
	double getEdgeResidualCap(int i, int j);
};
