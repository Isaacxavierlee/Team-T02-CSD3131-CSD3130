import numpy as np
import networkx as nx

def find_seam(energy_map: np.ndarray) -> list[int]:
    """
    Computes the optimal vertical seam using a max-flow/min-cut graph formulation.
    Horizontal seams are handled by the C++ backend passing a transposed energy map.
    """
    H, W = energy_map.shape
    G = nx.DiGraph()
    INF = float('inf')
    
    edges = []
    
    for r in range(H):
        # Connect Source ('S') to every pixel in the first column
        edges.append(('S', (r, 0), INF))
        
        # Connect every pixel in the last column to the Sink ('T')
        edges.append(((r, W - 1), 'T', INF))
        
        for c in range(W):
            # Forward edge (r, c) -> (r, c+1): Capacity is the pixel's energy
            # Severing this edge removes the pixel from the image
            if c < W - 1:
                edges.append(((r, c), (r, c + 1), float(energy_map[r, c])))
                
                # Infinite-capacity backward edge to prevent invalid reverse cuts
                edges.append(((r, c + 1), (r, c), INF))
            
            # Infinite-capacity diagonal backward edges to enforce a continuous seam
            if c > 0:
                if r > 0:
                    edges.append(((r, c), (r - 1, c - 1), INF))
                if r < H - 1:
                    edges.append(((r, c), (r + 1, c - 1), INF))

    # Bulk load edges to optimize NetworkX initialization speed
    G.add_weighted_edges_from(edges, weight='capacity')

    # Execute the max-flow algorithm to find the minimum s-t cut
    _, partition = nx.minimum_cut(G, 'S', 'T')
    
    # partition[0] contains all nodes reachable from the Source in the residual graph
    source_partition = partition[0]

    # In each row, the rightmost pixel remaining in the Source partition is the seam pixel
    seam = [0] * H
    for node in source_partition:
        if node != 'S':
            r, c = node
            if c > seam[r]:
                seam[r] = c

    return seam