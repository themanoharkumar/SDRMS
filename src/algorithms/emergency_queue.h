#ifndef EMERGENCY_QUEUE_H
#define EMERGENCY_QUEUE_H

#include <queue>
#include <string>
#include <vector>

/**
 * FIFO queue for incoming emergency requests (ambulance, food, boat, etc.).
 */
struct EmergencyRequest
{
    long long id = 0;
    std::string requestType;
    std::string location;
    std::string notes;
};

class EmergencyRequestQueue
{
public:
    void clear();
    void enqueue(const EmergencyRequest &req);
    bool dequeue(EmergencyRequest &out);
    bool front(EmergencyRequest &out) const;

    /** Cancel a specific request by its ID. Returns false if not found. */
    bool cancelById(long long id);

    std::vector<EmergencyRequest> pendingSnapshot() const;
    std::size_t size() const { return q_.size(); }

private:
    std::queue<EmergencyRequest> q_;
    mutable std::vector<EmergencyRequest> scratch_;
};

#endif // EMERGENCY_QUEUE_H
