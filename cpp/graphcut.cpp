#include "seam/graphcut.hpp"
#include <queue>
#include <algorithm>

int GraphCutSeamCarver::get_node(int r, int c) const {
    return r * W + c;
}

void GraphCutSeamCarver::add_edge(int from, int to, double cap) {
    adj[from].push_back({to, cap, 0.0, (int)adj[to].size()});
    adj[to].push_back({from, 0.0, 0.0, (int)adj[from].size() - 1});
}

bool GraphCutSeamCarver::bfs(std::vector<int>& parent, std::vector<int>& parent_edge) {
    std::fill(parent.begin(), parent.end(), -1);
    std::queue<int> q;
    
    q.push(S);
    parent[S] = -2; // Mark source as visited

    while (!q.empty()) {
        int curr = q.front();
        q.pop();

        if (curr == T) return true;

        for (int i = 0; i < adj[curr].size(); ++i) {
            const Edge& e = adj[curr][i];
            // If not visited and there is residual capacity
            if (parent[e.to] == -1 && e.capacity - e.flow > 0.0001) {
                parent[e.to] = curr;
                parent_edge[e.to] = i;
                q.push(e.to);
            }
        }
    }
    return false;
}

std::vector<int> GraphCutSeamCarver::find_vertical_seam(const std::vector<std::vector<double>>& energy_map) {
    H = energy_map.size();
    if (H == 0) return {};
    W = energy_map[0].size();
    
    S = H * W;
    T = H * W + 1;
    adj.assign(T + 1, std::vector<Edge>());

    // 1. Build the Graph
    for (int r = 0; r < H; ++r) {
        add_edge(S, get_node(r, 0), INF);
        add_edge(get_node(r, W - 1), T, INF);

        for (int c = 0; c < W; ++c) {
            if (c < W - 1) {
                add_edge(get_node(r, c), get_node(r, c + 1), energy_map[r][c]);
                add_edge(get_node(r, c + 1), get_node(r, c), INF);
            }
            if (c > 0) {
                if (r > 0) add_edge(get_node(r, c), get_node(r - 1, c - 1), INF);
                if (r < H - 1) add_edge(get_node(r, c), get_node(r + 1, c - 1), INF);
            }
        }
    }

    // 2. Edmonds-Karp Max-Flow Algorithm
    std::vector<int> parent(T + 1);
    std::vector<int> parent_edge(T + 1);

    while (bfs(parent, parent_edge)) {
        double bottleneck = INF;
        int curr = T;
        
        while (curr != S) {
            int p = parent[curr];
            int idx = parent_edge[curr];
            bottleneck = std::min(bottleneck, adj[p][idx].capacity - adj[p][idx].flow);
            curr = p;
        }

        curr = T;
        while (curr != S) {
            int p = parent[curr];
            int idx = parent_edge[curr];
            int rev_idx = adj[p][idx].rev_index;

            adj[p][idx].flow += bottleneck;
            adj[curr][rev_idx].flow -= bottleneck;
            curr = p;
        }
    }

    // 3. Find Minimum s-t Cut via BFS on the residual graph
    std::vector<bool> reachable(T + 1, false);
    std::queue<int> q;
    q.push(S);
    reachable[S] = true;

    while (!q.empty()) {
        int curr = q.front();
        q.pop();

        for (const Edge& e : adj[curr]) {
            if (!reachable[e.to] && e.capacity - e.flow > 0.0001) {
                reachable[e.to] = true;
                q.push(e.to);
            }
        }
    }

    // 4. Extract the Seam
    std::vector<int> seam(H, 0);
    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) {
            if (reachable[get_node(r, c)]) {
                seam[r] = std::max(seam[r], c);
            }
        }
    }

    return seam;
}