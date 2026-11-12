"""GRAPH CUT SEAM (min s-t cut / max-flow)  -  owner: Dexter     (Spec 4, Spec 5b/5c; Rubric: Design, Implementation)

SPEC 4 graph for a vertical seam (all built in build_network):
  * one node per pixel; source -> every pixel of the first column, every pixel of the last column -> sink,
    infinite capacity
  * edge (i,j)->(i,j+1) with capacity = cost of removing pixel (i,j)  (its energy e[i,j])
  * infinite backward edge (i,j+1)->(i,j)  => the S side of a finite cut is a PREFIX of every row
    => every finite cut crosses each row exactly once
  * infinite diagonal edges (i,j)->(i-1,j-1) and (i+1,j-1) => seam pixels of adjacent rows differ by <= 1
  Max-flow = min-cut (theorem), cut capacity = total seam energy => the seam is optimal (same cost as DP).
  Seam extraction: nodes reachable from the source in the RESIDUAL graph form S; in each row the
  rightmost S pixel is the seam pixel.   Horizontal seams: carver.py transposes the image (Spec 4).

DESIGN DECISION (report!): with the sink on the last column, edge (i,j)->(i,j+1) prices pixel j, so the
  last real column could never be removed. We append a ZERO-cost dummy column on the right.
  (found by our brute-force test, tests/test_all.py::test_dp_and_graphcut_are_optimal)

THREE MAX-FLOW ENGINES (compare in the report; all produce a minimum cut of equal capacity):
  edmonds_karp  Ford-Fulkerson with BFS shortest augmenting paths      O(V * E^2)   (the one named in Spec 4)
  dinic         BFS level graph + blocking flow                        O(V^2 * E)   (faster in practice)
  networkx      library reference, used only to cross-check in tests
  Here V = H*(W+1)+2 ~ H*W and E ~ 8*H*W residual arcs  => graph cut is orders of magnitude slower than
  DP's Theta(H*W). Space Theta(H*W) (Python lists - memory heavy).  Use --max-width for demos.

IMPROVE: pure numpy/C implementation, reuse flow between iterations, forward-energy capacities.
"""
import math
from collections import deque
import numpy as np
import networkx as nx

EPS = 1e-9
INF = math.inf


def build_network(energy: np.ndarray):
    """Returns (n, s, t, to, cap, adj, W). Edge e and e^1 are a forward/reverse residual pair."""
    h, w0 = energy.shape
    W = w0 + 1                                   # + dummy column
    s, t = h * W, h * W + 1
    n = h * W + 2
    to, cap = [], []
    adj = [[] for _ in range(n)]

    def add(u, v, c):
        adj[u].append(len(to)); to.append(v); cap.append(c)
        adj[v].append(len(to)); to.append(u); cap.append(0.0)

    for i in range(h):
        add(s, i * W, INF)
        add(i * W + W - 1, t, INF)
        for j in range(W - 1):
            u = i * W + j
            add(u, u + 1, float(energy[i, j]))   # cost of removing pixel (i, j)
            add(u + 1, u, INF)                   # backward edge
        for j in range(1, W):
            u = i * W + j
            if i > 0:
                add(u, (i - 1) * W + j - 1, INF)
            if i < h - 1:
                add(u, (i + 1) * W + j - 1, INF)
    return n, s, t, to, cap, adj, W


def edmonds_karp(n, s, t, to, cap, adj):
    """Ford-Fulkerson where each augmenting path is the SHORTEST one found by BFS."""
    flow = 0.0
    while True:
        parent = [-1] * n
        parent[s] = -2
        q, found = deque([s]), False
        while q and not found:
            u = q.popleft()
            for e in adj[u]:
                v = to[e]
                if parent[v] == -1 and cap[e] > EPS:
                    parent[v] = e
                    if v == t:
                        found = True
                        break
                    q.append(v)
        if not found:
            return flow
        f, v = INF, t
        while v != s:
            e = parent[v]; f = min(f, cap[e]); v = to[e ^ 1]
        v = t
        while v != s:
            e = parent[v]; cap[e] -= f; cap[e ^ 1] += f; v = to[e ^ 1]
        flow += f


def dinic(n, s, t, to, cap, adj):
    """Dinic: repeat {BFS levels; push blocking flow along level-increasing edges}. Iterative DFS."""
    flow = 0.0
    while True:
        level = [-1] * n
        level[s] = 0
        q = deque([s])
        while q:
            u = q.popleft()
            for e in adj[u]:
                if cap[e] > EPS and level[to[e]] < 0:
                    level[to[e]] = level[u] + 1
                    q.append(to[e])
        if level[t] < 0:
            return flow
        it = [0] * n
        while True:
            path, u = [], s
            while u != t:
                advanced = False
                while it[u] < len(adj[u]):
                    e = adj[u][it[u]]
                    v = to[e]
                    if cap[e] > EPS and level[v] == level[u] + 1:
                        path.append(e); u = v; advanced = True
                        break
                    it[u] += 1
                if not advanced:
                    if u == s:
                        break
                    e = path.pop(); u = to[e ^ 1]; it[u] += 1     # dead end: retreat
            if u != t:
                break
            f = min(cap[e] for e in path)
            for e in path:
                cap[e] -= f; cap[e ^ 1] += f
            flow += f


def source_side(n, s, to, cap, adj):
    """Nodes reachable from s in the residual graph = the S side of a minimum cut."""
    seen = [False] * n
    seen[s] = True
    q = deque([s])
    while q:
        u = q.popleft()
        for e in adj[u]:
            if cap[e] > EPS and not seen[to[e]]:
                seen[to[e]] = True
                q.append(to[e])
    return seen


def _seam_from_cut(seen, h, w0, W):
    S = np.array(seen[: h * W], dtype=bool).reshape(h, W)[:, :w0]
    return (w0 - 1 - np.argmax(S[:, ::-1], axis=1)).astype(np.int64)    # rightmost S pixel per row


def _find_seam_with(engine, energy):
    h, w0 = energy.shape
    n, s, t, to, cap, adj, W = build_network(energy)
    engine(n, s, t, to, cap, adj)
    return _seam_from_cut(source_side(n, s, to, cap, adj), h, w0, W)


def find_seam(energy):              # default engine: Dinic
    return _find_seam_with(dinic, energy)


def find_seam_edmonds_karp(energy):
    return _find_seam_with(edmonds_karp, energy)


def find_seam_networkx(energy):
    """Library reference implementation (cross-check only)."""
    h, w0 = energy.shape
    G = nx.DiGraph()                 # edge without 'capacity' = infinite
    e = np.pad(energy, ((0, 0), (0, 1)))
    W = w0 + 1
    for i in range(h):
        G.add_edge("s", (i, 0)); G.add_edge((i, W - 1), "t")
        for j in range(W - 1):
            G.add_edge((i, j), (i, j + 1), capacity=float(e[i, j])); G.add_edge((i, j + 1), (i, j))
        for j in range(1, W):
            if i > 0: G.add_edge((i, j), (i - 1, j - 1))
            if i < h - 1: G.add_edge((i, j), (i + 1, j - 1))
    _, (S, _T) = nx.minimum_cut(G, "s", "t")
    return np.array([max(j for j in range(w0) if (i, j) in S) for i in range(h)], dtype=np.int64)
