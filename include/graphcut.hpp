#ifndef GRAPHCUT_HPP
#define GRAPHCUT_HPP

#include <limits>
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
    const double INF = std::numeric_limits<double>::infinity();

    // Converts a row and graph column to a 1D node ID.
    int get_node(int r, int c) const;

    // Helper to add a directed edge with capacity and a residual backward edge
    void add_edge(int from, int to, double cap);

    // Breadth-First Search to find an augmenting path for Edmonds-Karp
    bool bfs(std::vector<int>& parent, std::vector<int>& parent_edge);

public:
    // Returns the minimum-energy seam for a rectangular, finite, nonnegative map.
    std::vector<int> find_vertical_seam(const std::vector<std::vector<double>>& energy_map);
};

#endif // GRAPHCUT_HPP
