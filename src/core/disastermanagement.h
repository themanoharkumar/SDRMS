#ifndef DISASTERMANAGEMENT_H
#define DISASTERMANAGEMENT_H

#include <string>
#include <vector>

/**
 * Disaster registration records (floods, earthquakes, fires, cyclones, landslides).
 */
struct DisasterRecord
{
    std::string disasterType;
    std::string location;
    int severity = 1; // 1-10
    int affectedPeople = 0;
    int teamsNeeded = 0;
    std::string status    = "Active";    // "Active" | "Resolved" | "Monitoring"
    std::string timestamp;               // ISO date-time when registered
};

class DisasterRegistry
{
public:
    void clear() { items_.clear(); }
    
    void registerDisaster(const DisasterRecord &d) { items_.push_back(d); }
    
    const std::vector<DisasterRecord> &records() const { return items_; }
    std::vector<DisasterRecord> &records() { return items_; }

    /** Remove the record at position idx (0-based). Returns false if out of range. */
    bool removeAt(int idx)
    {
        if (idx < 0 || idx >= static_cast<int>(items_.size()))
            return false;
        items_.erase(items_.begin() + idx);
        return true;
    }

private:
    std::vector<DisasterRecord> items_;
};

#endif // DISASTERMANAGEMENT_H
