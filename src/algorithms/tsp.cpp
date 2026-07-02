#include "tsp.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr double kInf = 1e100;
}

TspResult solveTspExact(const std::vector<std::vector<double>> &dist)
{
    TspResult res;
    const int n = static_cast<int>(dist.size());
    if (n == 0) {
        res.ok = true;
        return res;
    }
    if (n == 1) {
        res.tour = {0};
        res.tourCost = 0;
        res.ok = true;
        return res;
    }

    const int fullMask = (1 << n) - 1;
    std::vector<std::vector<double>> dp(1 << n, std::vector<double>(n, kInf));
    std::vector<std::vector<int>> parent(1 << n, std::vector<int>(n, -1));

    dp[1][0] = 0;

    for (int mask = 1; mask <= fullMask; ++mask) {
        for (int j = 0; j < n; ++j) {
            if (!(mask & (1 << j)))
                continue;
            double cur = dp[mask][j];
            if (cur >= kInf / 2)
                continue;
            for (int k = 0; k < n; ++k) {
                if (mask & (1 << k))
                    continue;
                int nmask = mask | (1 << k);
                double nd = cur + dist[j][k];
                if (nd < dp[nmask][k]) {
                    dp[nmask][k] = nd;
                    parent[nmask][k] = j;
                }
            }
        }
    }

    double best = kInf;
    int last = -1;
    for (int j = 1; j < n; ++j) {
        double c = dp[fullMask][j] + dist[j][0];
        if (c < best) {
            best = c;
            last = j;
        }
    }

    if (last < 0 || !std::isfinite(best)) {
        res.ok = false;
        return res;
    }

    res.tourCost = best;
    res.ok = true;

    // Reconstruct Hamiltonian path starting at 0 (Held–Karp parent pointers)
    std::vector<int> tourNodes;
    int mask = fullMask;
    int cur = last;
    while (cur != 0) {
        tourNodes.push_back(cur);
        int prev = parent[mask][cur];
        mask ^= (1 << cur);
        cur = prev;
    }
    tourNodes.push_back(0);
    std::reverse(tourNodes.begin(), tourNodes.end());
    res.tour = tourNodes;
    return res;
}
