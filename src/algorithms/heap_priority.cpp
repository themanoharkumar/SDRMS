#include "heap_priority.h"

void RescuePriorityHeap::clear()
{
    heap_.clear();
}

bool RescuePriorityHeap::higher(const PriorityTask &a, const PriorityTask &b)
{
    if (a.severity != b.severity)
        return a.severity > b.severity;
    return a.id < b.id;
}

void RescuePriorityHeap::siftUp(std::size_t i)
{
    while (i > 0) {
        std::size_t p = (i - 1) / 2;
        if (!higher(heap_[i], heap_[p]))
            break;
        std::swap(heap_[i], heap_[p]);
        i = p;
    }
}

void RescuePriorityHeap::siftDown(std::size_t i)
{
    const std::size_t n = heap_.size();
    for (;;) {
        std::size_t best = i;
        std::size_t l = 2 * i + 1;
        std::size_t r = 2 * i + 2;
        if (l < n && higher(heap_[l], heap_[best]))
            best = l;
        if (r < n && higher(heap_[r], heap_[best]))
            best = r;
        if (best == i)
            break;
        std::swap(heap_[i], heap_[best]);
        i = best;
    }
}

void RescuePriorityHeap::push(const PriorityTask &task)
{
    heap_.push_back(task);
    siftUp(heap_.size() - 1);
}

bool RescuePriorityHeap::pop(PriorityTask &out)
{
    if (heap_.empty())
        return false;
    out = heap_.front();
    heap_.front() = heap_.back();
    heap_.pop_back();
    if (!heap_.empty())
        siftDown(0);
    return true;
}

bool RescuePriorityHeap::peek(PriorityTask &out) const
{
    if (heap_.empty())
        return false;
    out = heap_.front();
    return true;
}

std::vector<PriorityTask> RescuePriorityHeap::snapshotLevels() const
{
    return heap_;
}
