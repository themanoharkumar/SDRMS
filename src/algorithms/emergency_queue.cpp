#include "emergency_queue.h"

void EmergencyRequestQueue::clear()
{
    while (!q_.empty())
        q_.pop();
}

void EmergencyRequestQueue::enqueue(const EmergencyRequest &req)
{
    q_.push(req);
}

bool EmergencyRequestQueue::dequeue(EmergencyRequest &out)
{
    if (q_.empty())
        return false;
    out = q_.front();
    q_.pop();
    return true;
}

bool EmergencyRequestQueue::front(EmergencyRequest &out) const
{
    if (q_.empty())
        return false;
    out = q_.front();
    return true;
}

bool EmergencyRequestQueue::cancelById(long long id)
{
    // Rebuild the queue excluding the matching entry.
    std::queue<EmergencyRequest> tmp;
    bool found = false;
    while (!q_.empty()) {
        EmergencyRequest req = q_.front();
        q_.pop();
        if (!found && req.id == id) {
            found = true; // skip (cancel) this one
        } else {
            tmp.push(req);
        }
    }
    q_ = std::move(tmp);
    return found;
}

std::vector<EmergencyRequest> EmergencyRequestQueue::pendingSnapshot() const
{
    scratch_.clear();
    std::queue<EmergencyRequest> tmp = q_;
    while (!tmp.empty()) {
        scratch_.push_back(tmp.front());
        tmp.pop();
    }
    return scratch_;
}
