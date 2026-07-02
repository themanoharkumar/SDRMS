#ifndef STACK_HISTORY_H
#define STACK_HISTORY_H

#include <stack>
#include <string>
#include <vector>

/**
 * LIFO stack for completed rescue operations (history + optional undo).
 */
struct HistoryEntry
{
    std::string timestamp;
    std::string operationType;
    std::string summary;
    std::string detail;
};

class RescueHistoryStack
{
public:
    void clear();
    void push(const HistoryEntry &e);
    bool pop(HistoryEntry &out); // undo / remove last
    bool top(HistoryEntry &out) const;

    /** Bottom-to-top order for display (oldest first). */
    std::vector<HistoryEntry> toVectorOldestFirst() const;

    std::size_t size() const { return st_.size(); }

private:
    std::stack<HistoryEntry> st_;
    mutable std::vector<HistoryEntry> tmp_;
};

#endif // STACK_HISTORY_H
