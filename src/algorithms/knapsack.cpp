#include "knapsack.h"
#include <algorithm>

KnapsackResult solveReliefKnapsack(const std::vector<ReliefItem> &items, int capacity)
{
    KnapsackResult result;
    const int n = static_cast<int>(items.size());
    if (n == 0 || capacity <= 0)
        return result;

    // dp[i][w] = max value using first i items with capacity w
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(capacity + 1, 0));

    for (int i = 1; i <= n; ++i) {
        const int wgt = items[i - 1].weight;
        const int val = items[i - 1].value;
        for (int w = 0; w <= capacity; ++w) {
            dp[i][w] = dp[i - 1][w];
            if (wgt <= w)
                dp[i][w] = std::max(dp[i][w], dp[i - 1][w - wgt] + val);
        }
    }

    result.maxValue = dp[n][capacity];

    // Backtrack selected items
    int w = capacity;
    for (int i = n; i >= 1; --i) {
        if (dp[i][w] != dp[i - 1][w]) {
            result.selectedIndices.push_back(i - 1);
            w -= items[i - 1].weight;
        }
    }
    std::reverse(result.selectedIndices.begin(), result.selectedIndices.end());

    result.usedWeight = 0;
    for (int idx : result.selectedIndices)
        result.usedWeight += items[idx].weight;

    return result;
}
