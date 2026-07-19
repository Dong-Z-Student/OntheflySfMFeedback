#include "dynamic_graph_cut.h"
#include "graph.cpp"
#include "maxflow.cpp"

DynamicGraphCut::DynamicGraphCut(int num_nodes, int estimated_edges)
	: num_nodes_(num_nodes), first_maxflow_(true) {
	if (estimated_edges == 0) {
		estimated_edges = num_nodes_ * 4;
	}
	graph_ = new GraphType(num_nodes_, estimated_edges);
	graph_->add_node(num_nodes_);
}

DynamicGraphCut::~DynamicGraphCut() {
	delete graph_;
}

void DynamicGraphCut::addEdge(int i, int j, double cap, double rev_cap) {
	GraphType::arc_id forward_arc = graph_->get_first_arc() + graph_->get_arc_num();
	graph_->add_edge(i, j, cap, rev_cap);
	GraphType::arc_id reverse_arc = forward_arc + 1;
	edge_map_[{i, j}] = { forward_arc, reverse_arc, i, j };
}

void DynamicGraphCut::setTerminalWeights(int i, double source_cap, double sink_cap) {
	graph_->add_tweights(i, source_cap, sink_cap);
}

void DynamicGraphCut::removeNode(int i)
{
	graph_->set_trcap(i, 0);

	std::vector<std::pair<int, int>> edges_to_remove;

	for (const auto& kv : edge_map_) {
		const EdgeInfo& edge = kv.second;

		if (edge.from == i || edge.to == i) {
			graph_->set_rcap(edge.forward_arc, 0);
			graph_->set_rcap(edge.reverse_arc, 0);

			if (edge.from != i) {
				graph_->mark_node(edge.from);
			}
			if (edge.to != i) {
				graph_->mark_node(edge.to);
			}

			edges_to_remove.push_back(kv.first);
		}
	}

	for (const auto& edge_key : edges_to_remove) {
		edge_map_.erase(edge_key);
	}

	graph_->mark_node(i);
}

void DynamicGraphCut::updateEdgeCapacity(int i, int j, double new_cap, double new_rev_cap) {
	auto it = edge_map_.find({ i, j });
	if (it != edge_map_.end()) {
		EdgeInfo& edge = it->second;
		graph_->set_rcap(edge.forward_arc, new_cap);
		graph_->set_rcap(edge.reverse_arc, new_rev_cap);
		graph_->mark_node(i);
		graph_->mark_node(j);
	}
}

void DynamicGraphCut::updateTerminalWeights(int i, double source_cap, double sink_cap) {
	double new_trcap = source_cap - sink_cap;
	graph_->set_trcap(i, new_trcap);
	graph_->mark_node(i);
}

void DynamicGraphCut::batchUpdateEdges(const std::vector<std::tuple<int, int, double, double>>& updates) {
	for (const auto& update : updates) {
		int i = std::get<0>(update);
		int j = std::get<1>(update);
		double cap = std::get<2>(update);
		double rev_cap = std::get<3>(update);
		updateEdgeCapacity(i, j, cap, rev_cap);
	}
}

double DynamicGraphCut::computeMaxFlow(bool reuse_trees, Block<int>* changed_list) {
	if (first_maxflow_) {
		first_maxflow_ = false;
		return graph_->maxflow(false, nullptr);
	}
	return graph_->maxflow(reuse_trees, changed_list);
}

int DynamicGraphCut::getSegment(int i) {
	return graph_->what_segment(i);
}

std::vector<std::pair<int, int>> DynamicGraphCut::getCutEdges() {
	std::vector<std::pair<int, int>> cut_edges;
	for (const auto& kv : edge_map_) {
		int i = kv.second.from;
		int j = kv.second.to;
		if (graph_->what_segment(i) != graph_->what_segment(j)) {
			cut_edges.push_back({ i, j });
		}
	}
	return cut_edges;
}

std::vector<int> DynamicGraphCut::getChangedNodes(Block<int>* changed_list) {
	std::vector<int> changed_nodes;
	for (int* ptr = changed_list->ScanFirst(); ptr; ptr = changed_list->ScanNext()) {
		int i = *ptr;
		changed_nodes.push_back(i);
		graph_->remove_from_changed_list(i);
	}
	return changed_nodes;
}

double DynamicGraphCut::getEdgeResidualCap(int i, int j) {
	auto it = edge_map_.find({ i, j });
	if (it != edge_map_.end()) {
		return graph_->get_rcap(it->second.forward_arc);
	}
	return -1;
}
