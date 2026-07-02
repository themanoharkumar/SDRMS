#include "stack_history.h"
#include <algorithm>

void RescueHistoryStack::clear()
{
    while (!st_.empty())
        st_.pop();
}

void RescueHistoryStack::push(const HistoryEntry &e)
{
    st_.push(e);
}

bool RescueHistoryStack::pop(HistoryEntry &out)
{
    if (st_.empty())
        return false;
    out = st_.top();
    st_.pop();
    return true;
}

bool RescueHistoryStack::top(HistoryEntry &out) const
{
    if (st_.empty())
        return false;
    out = st_.top();
    return true;
}

std::vector<HistoryEntry> RescueHistoryStack::toVectorOldestFirst() const
{
    tmp_.clear();
    std::stack<HistoryEntry> copy = st_;
    while (!copy.empty()) {
        tmp_.push_back(copy.top());
        copy.pop();
    }
    std::reverse(tmp_.begin(), tmp_.end());
    return tmp_;
}
