#ifndef GRAPH_H
#define GRAPH_H

#include <limits>
#include <string>
#include <utility>
#include <vector>

/**
 * Weighted undirected graph: roads between locations.
 * Dijkstra (shortest safe path), BFS/DFS for zone connectivity.
 */
class ReliefRoadGraph
{
public:
    int addNode(const std::string &name);
    void addEdge(int u, int v, double weight, bool blocked = false);

    std::vector<std::pair<int, double>> neighbors(int u) const;

    /** Dijkstra from source; respects blocked edges as disconnected. */
    std::pair<std::vector<double>, std::vector<int>> dijkstra(int source) const;

    /** Reconstruct path to target using parent from dijkstra. */
    static std::vector<int> pathFromParent(const std::vector<int> &parent, int target);

    /** BFS order from start on subgraph of edges with weight < blockThreshold (optional). */
    std::vector<int> bfsOrder(int start) const;

    /** DFS order (preorder) for zone scanning. */
    std::vector<int> dfsOrder(int start) const;

    /** Connected components: each vector is list of node ids in one component. */
    std::vector<std::vector<int>> connectedComponents() const;

    int nodeCount() const { return static_cast<int>(names_.size()); }
    std::string nodeName(int id) const;

    void setEdgeBlocked(int u, int v, bool blocked);
    /** Flip blocked flag if an undirected edge exists; returns false if edge missing. */
    bool toggleEdgeBlock(int u, int v);

private:
    struct Edge
    {
        int to;
        double w;
        bool blocked;
    };

    std::vector<std::string> names_;
    std::vector<std::vector<Edge>> adj_;

    void dfsVisit(int u, std::vector<char> &vis, std::vector<int> &order) const;
};

#endif // GRAPH_H
