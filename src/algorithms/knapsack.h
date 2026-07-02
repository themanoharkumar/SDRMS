#ifndef KNAPSACK_H
#define KNAPSACK_H

#include <string>
#include <vector>

/**
 * 0/1 Knapsack for relief cargo: maximize value under weight (capacity) limit.
 */
struct ReliefItem
{
    std::string name;
    int weight = 0;
    int value = 0;
};

struct KnapsackResult
{
    int maxValue = 0;
    int usedWeight = 0;
    std::vector<int> selectedIndices; // indices into input items
};

KnapsackResult solveReliefKnapsack(const std::vector<ReliefItem> &items, int capacity);

#endif // KNAPSACK_H
