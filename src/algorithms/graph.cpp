#include "graph.h"
#include <algorithm>
#include <queue>

int ReliefRoadGraph::addNode(const std::string &name)
{
    names_.push_back(name);
    adj_.emplace_back();
    return static_cast<int>(names_.size()) - 1;
}

void ReliefRoadGraph::addEdge(int u, int v, double weight, bool blocked)
{
    if (u < 0 || v < 0 || u >= nodeCount() || v >= nodeCount())
        return;
    adj_[u].push_back({v, weight, blocked});
    adj_[v].push_back({u, weight, blocked});
}

std::vector<std::pair<int, double>> ReliefRoadGraph::neighbors(int u) const
{
    std::vector<std::pair<int, double>> out;
    if (u < 0 || u >= nodeCount())
        return out;
    for (const Edge &e : adj_[u]) {
        if (!e.blocked)
            out.push_back({e.to, e.w});
    }
    return out;
}

void ReliefRoadGraph::setEdgeBlocked(int u, int v, bool blocked)
{
    if (u < 0 || v < 0 || u >= nodeCount() || v >= nodeCount())
        return;
    for (Edge &e : adj_[u])
        if (e.to == v)
            e.blocked = blocked;
    for (Edge &e : adj_[v])
        if (e.to == u)
            e.blocked = blocked;
}

bool ReliefRoadGraph::toggleEdgeBlock(int u, int v)
{
    if (u < 0 || v < 0 || u >= nodeCount() || v >= nodeCount())
        return false;
    bool found = false;
    bool newState = false;
    for (Edge &e : adj_[u]) {
        if (e.to == v) {
            e.blocked = !e.blocked;
            newState = e.blocked;
            found = true;
            break;
        }
    }
    if (!found)
        return false;
    for (Edge &e : adj_[v]) {
        if (e.to == u) {
            e.blocked = newState;
            break;
        }
    }
    return true;
}

std::pair<std::vector<double>, std::vector<int>> ReliefRoadGraph::dijkstra(int source) const
{
    const int n = nodeCount();
    const double inf = std::numeric_limits<double>::infinity();
    std::vector<double> dist(n, inf);
    std::vector<int> parent(n, -1);
    if (source < 0 || source >= n)
        return {dist, parent};

    using P = std::pair<double, int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
    dist[source] = 0;
    pq.push({0, source});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[u])
            continue;
        for (const Edge &e : adj_[u]) {
            if (e.blocked)
                continue;
            double nd = d + e.w;
            if (nd < dist[e.to]) {
                dist[e.to] = nd;
                parent[e.to] = u;
                pq.push({nd, e.to});
            }
        }
    }
    return {dist, parent};
}

std::vector<int> ReliefRoadGraph::pathFromParent(const std::vector<int> &parent, int target)
{
    std::vector<int> path;
    for (int v = target; v != -1; v = parent[v])
        path.push_back(v);
    std::reverse(path.begin(), path.end());
    if (!path.empty() && path.front() == -1)
        path.clear();
    return path;
}

std::vector<int> ReliefRoadGraph::bfsOrder(int start) const
{
    const int n = nodeCount();
    std::vector<int> order;
    if (start < 0 || start >= n)
        return order;
    std::vector<char> vis(n, 0);
    std::queue<int> q;
    vis[start] = 1;
    q.push(start);
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        order.push_back(u);
        for (const Edge &e : adj_[u]) {
            if (e.blocked || vis[e.to])
                continue;
            vis[e.to] = 1;
            q.push(e.to);
        }
    }
    return order;
}

void ReliefRoadGraph::dfsVisit(int u, std::vector<char> &vis, std::vector<int> &order) const
{
    vis[u] = 1;
    order.push_back(u);
    std::vector<Edge> edges = adj_[u];
    std::sort(edges.begin(), edges.end(), [](const Edge &a, const Edge &b) { return a.to < b.to; });
    for (const Edge &e : edges) {
        if (e.blocked || vis[e.to])
            continue;
        dfsVisit(e.to, vis, order);
    }
}

std::vector<int> ReliefRoadGraph::dfsOrder(int start) const
{
    const int n = nodeCount();
    std::vector<int> order;
    if (start < 0 || start >= n)
        return order;
    std::vector<char> vis(n, 0);
    dfsVisit(start, vis, order);
    return order;
}

std::vector<std::vector<int>> ReliefRoadGraph::connectedComponents() const
{
    const int n = nodeCount();
    std::vector<char> vis(n, 0);
    std::vector<std::vector<int>> comps;
    for (int i = 0; i < n; ++i) {
        if (vis[i])
            continue;
        std::vector<int> comp;
        std::queue<int> q;
        vis[i] = 1;
        q.push(i);
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            comp.push_back(u);
            for (const Edge &e : adj_[u]) {
                if (e.blocked || vis[e.to])
                    continue;
                vis[e.to] = 1;
                q.push(e.to);
            }
        }
        comps.push_back(std::move(comp));
    }
    return comps;
}

std::string ReliefRoadGraph::nodeName(int id) const
{
    if (id < 0 || id >= nodeCount())
        return {};
    return names_[id];
}

std::vector<ReliefRoadGraph::EdgeInfo> ReliefRoadGraph::allEdges() const
{
    std::vector<EdgeInfo> out;
    for (int u = 0; u < nodeCount(); ++u) {
        for (const auto &e : adj_[u]) {
            if (u < e.to) {
                out.push_back({u, e.to, e.w, e.blocked});
            }
        }
    }
    return out;
}

void ReliefRoadGraph::clear()
{
    names_.clear();
    adj_.clear();
}
