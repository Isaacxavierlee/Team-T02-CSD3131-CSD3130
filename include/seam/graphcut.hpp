#ifndef GRAPHCUT_HPP
#define GRAPHCUT_HPP

#include <vector>

struct Edge {
    int to;
    double capacity;
    double flow;
    int rev_index; // Index of the reverse edge in the adjacency list
};

class GraphCutSeamCarver {
private:
    int H, W;
    int S, T; // Source and Sink nodes
    std::vector<std::vector<Edge>> adj;
    const double INF = 1e9; // Representation of infinite capacity

    // Converts 2D pixel coordinates to a 1D graph node ID
    int get_node(int r, int c) const;

    // Helper to add a directed edge with capacity and a residual backward edge
    void add_edge(int from, int to, double cap);

    // Breadth-First Search to find an augmenting path for Edmonds-Karp
    bool bfs(std::vector<int>& parent, std::vector<int>& parent_edge);

public:
    // Takes a 2D energy map and returns the optimal seam (column indices to remove)
    std::vector<int> find_vertical_seam(const std::vector<std::vector<double>>& energy_map);
};

#endif // GRAPHCUT_HPP