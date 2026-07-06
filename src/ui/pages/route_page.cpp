#include "route_page.h"
#include "page_header.h"
#include "page_scroll.h"
#include "appdata.h"
#include "graph.h"
#include "tsp.h"
#include "graph_widget.h"
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <limits>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

RoutePage::RoutePage(ApplicationModel *model, QWidget *parent)
    : QWidget(parent)
    , model_(model)
{
    auto *root = pageScrollSetup(this);
    addSdrmsPanelHeader(root, 7, tr("Route optimization (TSP + Dijkstra)"),
                        tr("Shortest safe paths on the road graph; exact TSP for multi-stop convoy planning."));

    QHBoxLayout *splitLayout = new QHBoxLayout;
    QVBoxLayout *leftCol = new QVBoxLayout;

    auto *g1 = new QGroupBox(tr("Shortest safe path (Dijkstra)"));
    auto *lay1 = new QGridLayout(g1);
    from_ = new QComboBox;
    to_ = new QComboBox;
    lay1->addWidget(new QLabel(tr("From")), 0, 0);
    lay1->addWidget(from_, 0, 1);
    lay1->addWidget(new QLabel(tr("To")), 1, 0);
    lay1->addWidget(to_, 1, 1);
    auto *b1 = new QPushButton(tr("Compute shortest path"));
    connect(b1, &QPushButton::clicked, this, &RoutePage::runDijkstra);
    lay1->addWidget(b1, 2, 1);

    auto *g2 = new QGroupBox(tr("Road blockage simulation"));
    auto *lay2 = new QGridLayout(g2);
    blockA_ = new QComboBox;
    blockB_ = new QComboBox;
    lay2->addWidget(new QLabel(tr("Edge A")), 0, 0);
    lay2->addWidget(blockA_, 0, 1);
    lay2->addWidget(new QLabel(tr("Edge B")), 1, 0);
    lay2->addWidget(blockB_, 1, 1);
    auto *b2 = new QPushButton(tr("Toggle blocked status"));
    connect(b2, &QPushButton::clicked, this, &RoutePage::toggleBlock);
    lay2->addWidget(b2, 2, 1);

    auto *g3 = new QGroupBox(tr("Multi-location TSP tour"));
    auto *lay3 = new QVBoxLayout(g3);
    auto *b3 = new QPushButton(tr("Solve TSP on sample district distance matrix"));
    connect(b3, &QPushButton::clicked, this, &RoutePage::runTsp);
    lay3->addWidget(b3);

    leftCol->addWidget(g1);
    leftCol->addWidget(g2);
    leftCol->addWidget(g3);

    output_ = new QTextEdit;
    output_->setReadOnly(true);
    output_->setMinimumHeight(150);
    leftCol->addWidget(output_);

    graphWidget_ = new RoadGraphWidget(model_, this);

    splitLayout->addLayout(leftCol, 2);
    splitLayout->addWidget(graphWidget_, 3);
    root->addLayout(splitLayout);

    refresh();
}

void RoutePage::reloadCombos()
{
    from_->clear();
    to_->clear();
    blockA_->clear();
    blockB_->clear();
    const int n = model_->roadGraph.nodeCount();
    for (int i = 0; i < n; ++i) {
        QString label = QString::fromStdString(model_->roadGraph.nodeName(i));
        from_->addItem(label, i);
        to_->addItem(label, i);
        blockA_->addItem(label, i);
        blockB_->addItem(label, i);
    }
}

void RoutePage::refresh()
{
    reloadCombos();
    if (graphWidget_) {
        graphWidget_->clearHighlight();
        graphWidget_->update();
    }
}

void RoutePage::runDijkstra()
{
    if (from_->count() == 0) {
        QMessageBox::warning(this, tr("Graph"), tr("Road network not initialized."));
        return;
    }
    int s = from_->currentData().toInt();
    int t = to_->currentData().toInt();
    auto [dist, parent] = model_->roadGraph.dijkstra(s);
    if (dist[static_cast<std::size_t>(t)] >= std::numeric_limits<double>::infinity()) {
        output_->setPlainText(tr("No path — road network may be disconnected or blocked."));
        if (graphWidget_) graphWidget_->clearHighlight();
        QMessageBox::warning(this, tr("Routing"), tr("Target unreachable under current blockages."));
        return;
    }
    std::vector<int> path = ReliefRoadGraph::pathFromParent(parent, t);
    
    // Highlight route in vector map
    if (graphWidget_) graphWidget_->setHighlightedPath(path);

    QString line;
    line += tr("Distance: %1 km\nPath: ").arg(dist[static_cast<std::size_t>(t)], 0, 'f', 2);
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (i)
            line += " → ";
        line += QString::fromStdString(model_->roadGraph.nodeName(path[i]));
    }
    output_->setPlainText(line);
}

void RoutePage::toggleBlock()
{
    if (blockA_->count() == 0)
        return;
    int a = blockA_->currentData().toInt();
    int b = blockB_->currentData().toInt();
    if (a == b) {
        QMessageBox::information(this, tr("Roads"), tr("Select two different nodes."));
        return;
    }
    if (!model_->roadGraph.toggleEdgeBlock(a, b)) {
        QMessageBox::warning(this, tr("Roads"), tr("No direct road between those locations."));
        return;
    }
    
    if (graphWidget_) {
        graphWidget_->clearHighlight();
        graphWidget_->update();
    }

    output_->append(tr("Toggled blockage on selected road — re-run routing to evaluate impact."));
}

void RoutePage::runTsp()
{
    if (model_->tspDistMatrix.empty()) {
        QMessageBox::warning(this, tr("TSP"), tr("Distance matrix not loaded."));
        return;
    }
    TspResult r = solveTspExact(model_->tspDistMatrix);
    if (!r.ok) {
        if (graphWidget_) graphWidget_->clearHighlight();
        QMessageBox::warning(this, tr("TSP"), tr("Unable to solve TSP for current matrix."));
        return;
    }

    // Show TSP tour loop in visualizer
    if (graphWidget_) graphWidget_->setHighlightedPath(r.tour);

    QString txt;
    txt += tr("Optimal tour length: %1\nOrder: ").arg(r.tourCost, 0, 'f', 2);
    for (std::size_t i = 0; i < r.tour.size(); ++i) {
        if (i)
            txt += " → ";
        int idx = r.tour[i];
        if (idx >= 0 && idx < static_cast<int>(model_->tspCityLabels.size()))
            txt += QString::fromStdString(model_->tspCityLabels[static_cast<std::size_t>(idx)]);
        else
            txt += QString::number(idx);
    }
    txt += tr("\n(Return to Command Hub implied in cost.)");
    output_->setPlainText(txt);
}
