#include "shelter.h"
#include <algorithm>

void ShelterManager::clear()
{
    shelters_.clear();
}

void ShelterManager::addShelter(const ShelterRecord &s)
{
    shelters_.push_back(s);
}

int ShelterManager::nearestByCapacity(const std::string &hintLocation) const
{
    (void)hintLocation; // In a full GIS system we'd geocode; here pick max remaining capacity.
    int best = -1;
    int bestFree = -1;
    for (int i = 0; i < static_cast<int>(shelters_.size()); ++i) {
        int freeBeds = shelters_[i].capacity - shelters_[i].occupied;
        if (freeBeds > bestFree) {
            bestFree = freeBeds;
            best = i;
        }
    }
    return best;
}

bool ShelterManager::assignBeds(int shelterIndex, int beds)
{
    if (shelterIndex < 0 || shelterIndex >= static_cast<int>(shelters_.size()) || beds <= 0)
        return false;
    ShelterRecord &s = shelters_[shelterIndex];
    if (s.occupied + beds > s.capacity)
        return false;
    s.occupied += beds;
    return true;
}

bool ShelterManager::removeAt(int idx)
{
    if (idx < 0 || idx >= static_cast<int>(shelters_.size()))
        return false;
    shelters_.erase(shelters_.begin() + idx);
    return true;
}
