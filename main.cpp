#include <iostream>
#include <vector>
#include <list>
#include <memory>
#include <string>
#include <fstream>
#include <string_view>
#include <deque>
#include <iterator>
#include <algorithm>
#include <queue>


static std::string get_node_id(std::string_view str)
{
	int pos1 = str.find('"') + 1;
	int pos2 = str.find('"', pos1);
	return std::string{str.substr(pos1, pos2 - pos1)};
}

struct func_label {
	std::string label{};
	std::string title{};
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

	pos1 = str.find("node: { title: \"") + 16;
	if (pos1 != 16) {
		std::cerr << "Title is missing" << std::endl;
		std::terminate();
	}
	pos2 = str.find('"', pos1);
	node.title = std::string{str.substr(pos1, pos2 - pos1)};

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
			f_lbl.stack_usage = -2; // std::stoi(line.substr(tab_pos));
			break;
		}
	}
}

static void parse_stack_use(std::list<func_label>& labels, int argc, char* argv[])
{
	for (auto& lbl : labels) {
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
	std::string title{};
	int stack_use{};
	bool visited{};
	std::list<func_node*> calls{};
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

func_node& find_node(std::deque<func_node>& nodes, std::string title)
{
	for (auto& n : nodes) {
		if (title == n.title) {
			return n;
		}
	}
	std::cerr << "Edge not found" << std::endl;
	std::terminate();
}

static bool breadth_first_search(func_node* initial_node)
{
	std::queue<func_node*> queue{};
	for (auto node : initial_node->calls) {
		queue.emplace(node);
	}
	
	bool cycle_detected{false};
	while (!queue.empty()) {
		func_node* node = queue.front();
		queue.pop();
		if (node == initial_node) {
			cycle_detected = true;
			break;
		}
		node->visited = true;
		for (auto call : node->calls) {
			if (!call->visited) {
				queue.emplace(call);
			}
		}
	}

	return cycle_detected;
}

static bool HasCycle(std::deque<func_node>& graph)
{
	auto clear_visit = [](func_node& fn) { fn.visited = false; };
	bool cycle_detected{false};
	for (auto& node : graph) {
		std::for_each(graph.begin(), graph.end(), clear_visit);
		cycle_detected = breadth_first_search(&node);
		if (cycle_detected) {
			break;
		}
	}
	return cycle_detected;
}

static std::deque<func_node> all_nodes{};

int main(int argc, char* argv[])
{
	if (argc > 1) {
		parsed_nodes pn = parse_nodes(argc, argv);
		if (pn.status != 0) {
			std::cerr << "Indirect calls are not supported" << std::endl;
			std::terminate();
		}

		parse_stack_use(pn.labels, argc, argv);
		auto cmp = [](const func_label& a, const func_label& b) {return a.label < b.label;};
		auto eq = [](const func_label& a, const func_label& b) {return a.label == b.label;};
		pn.labels.sort(cmp);
		pn.labels.unique(eq);
		for (const auto& fl : pn.labels) {
			if (fl.stack_usage == -2) {
				continue;
			}
			func_node fn{};
			fn.name = fl.label;
			fn.stack_use = fl.stack_usage;
			fn.title = fl.title;
			// std::cout << fn.title << std::endl;
			all_nodes.emplace_front(fn);
		}
		std::cout << all_nodes.size() << std::endl;
		for (const auto& ed : pn.edges) {
			func_node& parent = find_node(all_nodes, ed.first);
			func_node& child = find_node(all_nodes, ed.second);
			parent.calls.emplace_back(&child);
		}
	}

	if (HasCycle(all_nodes)) {
		std::cerr << "Recursion detected! Terminating..." << std::endl;
		std::terminate();
	} else {
		std::cerr << "No cycle detected" << std::endl;
	}

	for (const auto& n : all_nodes) {
		depth_first_search(n, std::list<const func_node*>{&n});
	}

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
