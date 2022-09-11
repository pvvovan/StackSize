#include <iostream>
#include <vector>
#include <list>
#include <memory>
#include <string>
#include <fstream>
#include <string_view>


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
	return -1;
}

struct parsed_nodes {
	std::list<std::string> nodes{};
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
			// std::cout << line << std::endl;
			while (std::getline(file_ci, line)) {
				if (line.substr(0, 16).compare("node: { title: \"") == 0) {
					std::string node_id = get_node_id(line);
					if (node_id.compare("__indirect_call") == 0) {
						std::cerr << "indirect call" << std::endl;
						pn.status = 2;
						return pn;
					}
					// std::cout << node_id << std::endl;
					pn.nodes.emplace_back(node_id);
				} else if (line.substr(0, 21).compare("edge: { sourcename: \"") == 0) {
					// std::cout << get_id(line) << std::endl;
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
		}
	}
	return pn;
}

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
	}
	return 0;
}
