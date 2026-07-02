#ifndef TSP_H
#define TSP_H

#include <limits>
#include <vector>

/**
 * Exact TSP via Held–Karp (DP over subsets). O(n^2 * 2^n).
 * Suitable for small n (relief stops). Returns optimal tour length and order.
 */
struct TspResult
{
    double tourCost = 0;
    std::vector<int> tour; // node indices 0..n-1, cycle
    bool ok = false;
};

TspResult solveTspExact(const std::vector<std::vector<double>> &dist);

#endif // TSP_H
