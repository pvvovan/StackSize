#include <iostream>
#include <vector>
#include <list>
#include <memory>

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

int main(int argc, char* argv[])
{
	// Create a graph given in the above diagram
	Graph g(4);
	g.addEdge(0, 1);
	g.addEdge(0, 2);
	g.addEdge(1, 2);
	// g.addEdge(2, 0);
	g.addEdge(2, 3);
	// g.addEdge(3, 3);
	if (g.isCyclic()) {
		std::cout << "Graph contains cycle" << std::endl;
	} else {
		std::cout << "Graph doesn't contain cycle" << std::endl;
	}


	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			std::cout << argv[i] << std::endl;
		}
	}
	std::cout << "Stack size" << std::endl;
	return 0;
}
