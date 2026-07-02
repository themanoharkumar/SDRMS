#ifndef APPDATA_H
#define APPDATA_H

#include "disastermanagement.h"
#include "emergency_queue.h"
#include "graph.h"
#include "heap_priority.h"
#include "knapsack.h"
#include "shelter.h"
#include "stack_history.h"
#include <QString>
#include <vector>

/**
 * Central application state: bridges GUI and DSA/backend modules.
 */
class ApplicationModel
{
public:
    ApplicationModel();

    DisasterRegistry disasters;
    EmergencyRequestQueue emergencyQueue;
    RescuePriorityHeap priorityHeap;
    ShelterManager shelters;
    RescueHistoryStack history;

    ReliefRoadGraph roadGraph;
    std::vector<std::vector<double>> tspDistMatrix;
    std::vector<std::string> tspCityLabels;

    void loadSampleData();
    void resetIds();
    /** After loading from file, avoid ID collisions for new records. */
    void reseedCountersFromState();

    long long nextEmergencyId() { return ++emergencyId_; }
    long long nextPriorityId() { return ++priorityId_; }

    QString dataDirectory() const;

private:
    long long emergencyId_ = 1000;
    long long priorityId_ = 5000;
};

#endif // APPDATA_H
