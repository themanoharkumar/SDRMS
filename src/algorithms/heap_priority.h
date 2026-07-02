#ifndef HEAP_PRIORITY_H
#define HEAP_PRIORITY_H

#include <string>
#include <vector>

/**
 * Max-heap for rescue priority: higher severity is served first.
 * Backend DSA module (no Qt).
 */
struct PriorityTask
{
    long long id = 0;
    int severity = 0; // 1-10, higher = more urgent
    std::string category;
    std::string description;
};

class RescuePriorityHeap
{
public:
    void clear();
    void push(const PriorityTask &task);
    bool pop(PriorityTask &out); // removes max
    bool peek(PriorityTask &out) const;
    bool empty() const { return heap_.empty(); }
    std::size_t size() const { return heap_.size(); }

    /** Snapshot for GUI (sorted by heap order - not full sort). */
    std::vector<PriorityTask> snapshotLevels() const;

private:
    std::vector<PriorityTask> heap_;
    void siftUp(std::size_t i);
    void siftDown(std::size_t i);
    static bool higher(const PriorityTask &a, const PriorityTask &b);
};

#endif // HEAP_PRIORITY_H
