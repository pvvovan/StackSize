#include <iostream>
#include <vector>
#include <list>
#include <memory>
#include <string>
#include <fstream>
#include <string_view>
#include <deque>
#include <iterator>


class Graph
{
	int V;    // No. of vertices
	std::vector<std::list<int>> adj{};    // Pointer to an array containing adjacency lists
	bool isCyclicUtil(int v, bool visited[], bool *rs);  // used by isCyclic()
public:
	Graph(int V);   // Constructor
	void addEdge(int v, int w);   // to add an edge to graph
	bool isCyclic();    // returns true if there is a cycle in this graph
};

Graph::Graph(int V)
{
	this->V = V;
	adj.reserve(V);
	for (int i = 0; i < V; i++) {
		adj.push_back(std::list<int>{});
	}
}

void Graph::addEdge(int v, int w)
{
	adj[v].push_back(w); // Add w to v’s list.
}

// This function is a variation of DFSUtil() in 
// https://www.geeksforgeeks.org/archives/18212
bool Graph::isCyclicUtil(int v, bool visited[], bool *recStack)
{
	if (visited[v] == false) {
		// Mark the current node as visited and part of recursion stack
		visited[v] = true;
		recStack[v] = true;

		// Recur for all the vertices adjacent to this vertex
		std::list<int>::iterator i;
		for (i = adj[v].begin(); i != adj[v].end(); ++i) {
			if ( !visited[*i] && isCyclicUtil(*i, visited, recStack) ) {
				return true;
			} else if (recStack[*i]) {
				return true;
			} else {
				// Do nothing
			}
		}

	}
	recStack[v] = false;  // remove the vertex from recursion stack
	return false;
}

// Returns true if the graph contains a cycle, else false.
// This function is a variation of DFS() in 
// https://www.geeksforgeeks.org/archives/18212
bool Graph::isCyclic()
{
	// Mark all the vertices as not visited and not part of recursion stack
	std::unique_ptr<bool[]> visited = std::make_unique<bool[]>(V);
	std::unique_ptr<bool[]> recStack = std::make_unique<bool[]>(V);
	for (int i = 0; i < V; i++) {
		visited[i] = false;
		recStack[i] = false;
	}

	// Call the recursive helper function to detect cycle in different DFS trees
	for (int i = 0; i < V; i++) {
		if ( !visited[i] && isCyclicUtil(i, &visited[0], &recStack[0])) {
			return true;
		}
	}

	return false;
}

static std::string get_node_id(std::string_view str)
{
	int pos1 = str.find('"') + 1;
	int pos2 = str.find('"', pos1);
	return std::string{str.substr(pos1, pos2 - pos1)};
}

struct func_label {
	std::string label{};
	bool external{};
	std::string translation_unit{};
	int stack_usage{-1};
};

static func_label get_node(std::string_view str)
{
	func_label node{};
	node.external = false;
	int pos1 = str.find("label: \"") + 8;
	int pos2 = str.find('"', pos1);
	node.label = std::string{str.substr(pos1, pos2 - pos1)};
	int pos3 = str.find("shape : ellipse", pos2);
	if (pos3 > 0) {
		node.external = true;
	}
	return node;
}

static std::pair<std::string, std::string> get_edge_ids(std::string_view str)
{
	std::pair<std::string, std::string> res{};

	int pos1 = str.find('"') + 1;
	int pos2 = str.find('"', pos1);
	res.first = std::string{str.substr(pos1, pos2 - pos1)};

	int pos3 = str.find('"', pos2 + 1) + 1;
	int pos4 = str.find('"', pos3);
	res.second = std::string{str.substr(pos3, pos4 - pos3)};

	return res;
}

static int find_pos(const std::list<std::string>& nodes, std::string_view node)
{
	int pos{0};
	for (auto n : nodes) {
		if (n.compare(node) == 0) {
			return pos;
		}
		pos++;
	}
	std::cerr << "Node not found" << std::endl;
	std::terminate();
}

struct parsed_nodes {
	std::list<std::string> nodes{};
	std::list<func_label> labels{};
	std::list<std::pair<std::string, std::string>> edges{};
	int status{0};
};

static parsed_nodes parse_nodes(int argc, char* argv[])
{
	parsed_nodes pn{};
	for (int i = 1; i < argc; i++) {
		std::string filename = argv[i];
		std::fstream file_ci(filename + ".ci", std::fstream::in);
		std::string line{};
		std::getline(file_ci, line);
		if (line.substr(0, 17).compare("graph: { title: \"") == 0) {
			while (std::getline(file_ci, line)) {
				if (line.substr(0, 16).compare("node: { title: \"") == 0) {
					std::string node_id = get_node_id(line);
					if (node_id.compare("__indirect_call") == 0) {
						std::cerr << "indirect call" << std::endl;
						pn.status = 2;
						return pn;
					}
					pn.nodes.emplace_back(node_id);
					auto node = get_node(line);
					node.translation_unit = filename;
					pn.labels.emplace_back(node);
				} else if (line.substr(0, 21).compare("edge: { sourcename: \"") == 0) {
					pn.edges.emplace_back(get_edge_ids(line));
				} else if (line.compare("}") == 0) {
					break;
				} else {
					std::cerr << "not supported" << std::endl;
					pn.status = 1;
					return pn;
				}
			}
		} else {
			pn.status = 3;
			return pn;
		}
	}
	return pn;
}

static std::string get_subline(const std::string_view label)
{
	std::size_t n = label.find("\\n") + 2;
	std::string subline {label.substr(n, label.size() - n)};
	subline += ':';

	n -= 2;
	subline += label.substr(0, n);
	return subline;
}

static std::string get_func_signature(const std::string_view label)
{
	std::size_t n = label.find("\\n");
	return std::string{label.substr(0, n)};
}

static void find_in_su(func_label& f_lbl, std::string file_prefix)
{
	std::string func_signature = get_func_signature(f_lbl.label);
	std::fstream file_su(file_prefix + ".su", std::fstream::in);
	std::string line{};
	while (std::getline(file_su, line)) {
		auto pos = line.find(func_signature);
		if (pos != std::string::npos) {
			auto tab_pos = line.find('\t');
			if (tab_pos == std::string::npos) {
				std::cerr << "TAB not found" << std::endl;
				std::terminate();
			}
			tab_pos += 1;
			f_lbl.stack_usage = std::stoi(line.substr(tab_pos));
			break;
		}
	}
}

static void parse_stack_use(parsed_nodes& pn, int argc, char* argv[])
{
	for (auto& lbl : pn.labels) {
		if (lbl.external == false) {
			std::string su_subline = get_subline(lbl.label);
			std::fstream file_su(lbl.translation_unit + ".su", std::fstream::in);
			std::string line{};
			bool su_exists{false};
			while (std::getline(file_su, line)) {
				auto pos = line.find(su_subline);
				if (pos != std::string::npos) {
					auto tab_pos = line.find('\t');
					if (tab_pos == std::string::npos) {
						std::cerr << "TAB not found" << std::endl;
						std::terminate();
					}
					tab_pos += 1;
					lbl.stack_usage = std::stoi(line.substr(tab_pos));
					su_exists = true;
					break;
				}
			}
			if (su_exists == false) {
				std::cerr << "Stack usage not found" << std::endl;
				std::terminate();
			}
		} else {
			for (int i = 1; i < argc; i++) {
				find_in_su(lbl, argv[i]);
			}
		}
		if (lbl.stack_usage == -1) {
			std::cout << "Default 8 bytes stack is for: " << lbl.label << std::endl;
			lbl.stack_usage = 8;
		}
	}
}

class func_node {
public:
	std::string name{};
	int stack_use{};
	std::list<const func_node*> calls{};
};

std::list<std::list<const func_node*>> all_routes{};
void depth_first_search(const func_node& node, std::list<const func_node*> route)
{
	if (node.calls.size() == 0) {
		all_routes.push_back(route);
	} else {
		for (auto c_node : node.calls) {
			std::list<const func_node*> c_route{route};
			c_route.push_back(c_node);
			depth_first_search(*c_node, c_route);
		}
	}
}

static std::deque<func_node> all_nodes{};

int main(int argc, char* argv[])
{
	if (argc > 1) {
		parsed_nodes pn = parse_nodes(argc, argv);
		if (pn.status == 0) {
			// check graph for recursion
			Graph call_graph(pn.nodes.size());
			for (const auto& p : pn.edges) {
				int first = find_pos(pn.nodes, p.first);
				int second = find_pos(pn.nodes, p.second);
				call_graph.addEdge(first, second);
			}
			if (call_graph.isCyclic()) {
				std::cout << "Recursion detected" << std::endl;
			} else {
				std::cout << "No recursion detected" << std::endl;
			}
		}

		parse_stack_use(pn, argc, argv);
		for (const auto& fl : pn.labels) {
			func_node fn{};
			fn.name = fl.label;
			fn.stack_use = fl.stack_usage;
			all_nodes.emplace_front(fn);
		}
		for (const auto& ed : pn.edges) {
			std::cout << ed.first << " " << ed.second << std::endl;
		}
	}

	all_nodes.clear();
	for (int i = 0; i < 8; i++) {
		all_nodes.emplace_front(func_node{});
	}

	all_nodes[0].name = "0";
	all_nodes[0].stack_use = 12;
	all_nodes[0].calls.emplace_back(&all_nodes[1]);

	all_nodes[1].name = "1";
	all_nodes[1].stack_use = 32;
	all_nodes[1].calls.emplace_back(&all_nodes[2]);

	all_nodes[2].name = "2";
	all_nodes[2].stack_use = 18;

	all_nodes[3].name = "3";
	all_nodes[3].stack_use = 24;
	all_nodes[3].calls.emplace_back(&all_nodes[0]);
	all_nodes[3].calls.emplace_back(&all_nodes[1]);
	all_nodes[3].calls.emplace_back(&all_nodes[2]);

	all_nodes[4].name = "4";
	all_nodes[4].stack_use = 4;
	all_nodes[4].calls.emplace_back(&all_nodes[3]);
	all_nodes[4].calls.emplace_back(&all_nodes[5]);

	all_nodes[5].name = "5";
	all_nodes[5].stack_use = 8;
	all_nodes[5].calls.emplace_back(&all_nodes[3]);
	all_nodes[5].calls.emplace_back(&all_nodes[6]);

	all_nodes[6].name = "6";
	all_nodes[6].stack_use = 16;
	all_nodes[6].calls.emplace_back(&all_nodes[2]);
	all_nodes[6].calls.emplace_back(&all_nodes[3]);
	all_nodes[6].calls.emplace_back(&all_nodes[7]);

	all_nodes[7].name = "7";
	all_nodes[7].stack_use = 32;

	// for (int i = 0; i < 8; i++) {
	// 	depth_first_search(all_nodes[i], std::list<const func_node*>{&all_nodes[i]});
	// }

	// auto print_routes = []() {
	// 	for (auto r : all_routes) {
	// 		for (auto f : r) {
	// 			std::cout << f->name << " ";
	// 		}
	// 		std::cout << std::endl;
	// 	}
	// };

	for (const auto& n : all_nodes) {
		depth_first_search(n, std::list<const func_node*>{&n});
	}

	// print_routes();

	auto find_max = []() {
		int max{0};
		int pos{0};
		int max_pos{0};
		for (const auto& r : all_routes) {
			int total{0};
			for (auto el : r) {
				total += el->stack_use;
			}
			if (total > max) {
				max = total;
				max_pos = pos;
			}
			pos++;
		}

		auto ri = all_routes.cbegin();
		std::advance(ri, max_pos);
		std::size_t ind{0};
		for (auto el = ri->cbegin(); el != ri->cend(); el++) {
			for (std::size_t i{0}; i < ind; i++) {
				std::cout << " ";
			}
			ind++;
			std::cout << (*el)->name << " (" << (*el)->stack_use << " bytes)";
			if (ind < ri->size()) {
				std::cout << " -> " << std::endl;
			} else {
				std::cout << std::endl;
			}
		}
		std::cout << "Max stack usage is " << max << " bytes!!!" << std::endl;
	};
	find_max();

	return 0;
}
