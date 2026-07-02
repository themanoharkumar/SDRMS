#ifndef SHELTER_H
#define SHELTER_H

#include <string>
#include <vector>

struct ShelterRecord
{
    std::string name;
    std::string location;
    int capacity = 0;
    int occupied = 0;
};

class ShelterManager
{
public:
    void clear();
    void addShelter(const ShelterRecord &s);
    const std::vector<ShelterRecord> &all() const { return shelters_; }
    std::vector<ShelterRecord> &all() { return shelters_; }

    /** Index of shelter with most free beds, or -1. */
    int nearestByCapacity(const std::string &hintLocation) const;

    bool assignBeds(int shelterIndex, int beds);

    /** Remove the shelter at position idx (0-based). Returns false if out of range. */
    bool removeAt(int idx);

private:
    std::vector<ShelterRecord> shelters_;
};

#endif // SHELTER_H
