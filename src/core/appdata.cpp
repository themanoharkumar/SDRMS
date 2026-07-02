#include "appdata.h"
#include <algorithm>
#include <QCoreApplication>
#include <QDir>

ApplicationModel::ApplicationModel()
{
    loadSampleData();
}

QString ApplicationModel::dataDirectory() const
{
    QString base = QCoreApplication::applicationDirPath();
    QDir dir(base);
    return dir.absoluteFilePath(QStringLiteral("resources"));
}

void ApplicationModel::resetIds()
{
    emergencyId_ = 1000;
    priorityId_ = 5000;
}

void ApplicationModel::reseedCountersFromState()
{
    long long maxE = emergencyId_;
    for (const auto &e : emergencyQueue.pendingSnapshot())
        maxE = std::max(maxE, e.id);
    emergencyId_ = std::max(emergencyId_, maxE);

    long long maxP = priorityId_;
    for (const auto &p : priorityHeap.snapshotLevels())
        maxP = std::max(maxP, p.id);
    priorityId_ = std::max(priorityId_, maxP);
}

void ApplicationModel::loadSampleData()
{
    disasters.clear();
    emergencyQueue.clear();
    priorityHeap.clear();
    shelters.clear();
    history.clear();
    roadGraph = ReliefRoadGraph();
    tspDistMatrix.clear();
    tspCityLabels.clear();
    resetIds();

    disasters.registerDisaster({"Flood", "East Riverside", 8, 1200, 12});
    disasters.registerDisaster({"Earthquake", "Downtown District", 9, 3500, 25});
    disasters.registerDisaster({"Wildfire", "Northern Ridge", 7, 800, 18});

    emergencyQueue.enqueue({nextEmergencyId(), "Ambulance", "Block 4", "ICU patient trapped"});
    emergencyQueue.enqueue({nextEmergencyId(), "Food", "Shelter A", "200 families"});
    emergencyQueue.enqueue({nextEmergencyId(), "Rescue boat", "Canal Zone", "High water"});

    priorityHeap.push({nextPriorityId(), 10, "Medical", "ICU patient - oxygen critical"});
    priorityHeap.push({nextPriorityId(), 8, "Child rescue", "School wing collapse"});
    priorityHeap.push({nextPriorityId(), 6, "Senior evacuation", "Apartment stairwell blocked"});

    shelters.addShelter({"Central Relief Hall", "City Center", 500, 120});
    shelters.addShelter({"Sports Arena Shelter", "West End", 800, 310});
    shelters.addShelter({"Community School", "North Hills", 400, 90});

    // Road graph nodes
    int hub = roadGraph.addNode("Command Hub");
    int a = roadGraph.addNode("District A");
    int b = roadGraph.addNode("District B");
    int c = roadGraph.addNode("District C");
    int d = roadGraph.addNode("Safe Zone North");
    roadGraph.addEdge(hub, a, 4.0, false);
    roadGraph.addEdge(hub, b, 7.0, false);
    roadGraph.addEdge(a, b, 3.0, false);
    roadGraph.addEdge(b, c, 5.0, false);
    roadGraph.addEdge(c, d, 6.0, false);
    roadGraph.addEdge(a, d, 12.0, false);

    // TSP distance matrix (symmetric) for same districts + hub
    tspCityLabels = {"Command Hub", "District A", "District B", "District C", "Safe Zone North"};
    const int n = 5;
    tspDistMatrix.assign(n, std::vector<double>(n, 0.0));
    auto setD = [&](int i, int j, double v) {
        tspDistMatrix[i][j] = v;
        tspDistMatrix[j][i] = v;
    };
    setD(0, 1, 4);
    setD(0, 2, 7);
    setD(1, 2, 3);
    setD(2, 3, 5);
    setD(3, 4, 6);
    setD(1, 4, 12);

    history.push({"2026-05-01 08:00", "Dispatch", "Medical team to District A", "3 units deployed"});
    history.push({"2026-05-01 09:30", "Supply", "Food drop Zone B", "Knapsack plan executed"});
}
