#include "overview_page.h"
#include "hover_scroll_area.h"
#include "page_header.h"
#include "appdata.h"
#include "knapsack.h"
#include "tsp.h"
#include <QAbstractItemView>
#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

namespace {

/**
 * Builds a KPI tile card. When the "View details" link label is clicked,
 * the tile emits navigateRequested(targetPage) via the provided callback.
 */
QFrame *makeKpiTile(const QString &objectName, const QString &title,
                    QLabel **valueOut, const QString &linkText,
                    OverviewPage *page, int targetPage)
{
    auto *card = new QFrame;
    card->setObjectName(objectName);
    card->setMinimumHeight(118);
    card->setMinimumWidth(128);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(16, 14, 16, 14);
    lay->setSpacing(6);
    auto *t = new QLabel(title);
    t->setObjectName(QStringLiteral("kpiTileTitle"));
    auto *v = new QLabel(QStringLiteral("0"));
    v->setObjectName(QStringLiteral("kpiTileValue"));
    auto *lnk = new QLabel(QStringLiteral("<a href='#'>") + linkText + QStringLiteral("</a>"));
    lnk->setObjectName(QStringLiteral("kpiTileLink"));
    lnk->setCursor(Qt::PointingHandCursor);
    QObject::connect(lnk, &QLabel::linkActivated, page, [page, targetPage](const QString &) {
        emit page->navigateRequested(targetPage);
    });
    lay->addWidget(t);
    lay->addWidget(v);
    lay->addStretch();
    lay->addWidget(lnk);
    *valueOut = v;
    return card;
}

std::vector<ReliefItem> sampleReliefCatalog()
{
    return {{"Food packets (pallet)", 15, 45}, {"Medicine crates", 10, 60}, {"Oxygen cylinders set", 25, 90},
            {"Rescue kits", 8, 35},           {"Portable hospital module", 40, 95}, {"Water tanks", 18, 40},
            {"Drone survey pack", 6, 30},     {"Ambulance supply bundle", 35, 85}};
}

QString severityLabel(int s)
{
    if (s >= 9)
        return QStringLiteral("Critical");
    if (s >= 7)
        return QStringLiteral("High");
    if (s >= 5)
        return QStringLiteral("Medium");
    return QStringLiteral("Low");
}

} // namespace

OverviewPage::OverviewPage(ApplicationModel *model, QWidget *parent)
    : QWidget(parent)
    , model_(model)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *scroll = new HoverRevealScrollArea;
    scroll->setObjectName(QStringLiteral("dashboardScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *body = new QWidget;
    auto *root = new QVBoxLayout(body);
    root->setContentsMargins(0, 0, 8, 0);
    root->setSpacing(20);

    addSdrmsPanelHeader(root, 2, tr("Dashboard"),
                        tr("Executive overview — KPIs, live feeds, and cross-module status."));

    dashDateLabel_ = new QLabel;
    dashDateLabel_->setObjectName(QStringLiteral("dashDateStrip"));
    root->addWidget(dashDateLabel_);

    // KPI tiles — each tile's "View details" link navigates to the matching page index
    auto *kpiRow = new QHBoxLayout;
    kpiRow->setSpacing(14);
    kpiRow->addWidget(makeKpiTile(QStringLiteral("kpiTileDisasters"), tr("Active disasters"),
                                  &kpiDisastersVal_, tr("View details →"), this, 1), 1);
    kpiRow->addWidget(makeKpiTile(QStringLiteral("kpiTileAffected"), tr("Affected people"),
                                  &kpiAffectedVal_, tr("View details →"), this, 1), 1);
    kpiRow->addWidget(makeKpiTile(QStringLiteral("kpiTileTeams"), tr("Rescue teams (planned)"),
                                  &kpiTeamsVal_, tr("View priority →"), this, 3), 1);
    kpiRow->addWidget(makeKpiTile(QStringLiteral("kpiTileResources"), tr("Shelter capacity free"),
                                  &kpiResourcesVal_, tr("View shelters →"), this, 6), 1);
    kpiRow->addWidget(makeKpiTile(QStringLiteral("kpiTileShelters"), tr("Shelters open"),
                                  &kpiSheltersVal_, tr("View shelters →"), this, 6), 1);
    root->addLayout(kpiRow);

    auto *midRow = new QHBoxLayout;
    midRow->setSpacing(16);

    auto *g1 = new QGroupBox(tr("Active disasters"));
    auto *l1 = new QVBoxLayout(g1);
    disasterFeed_ = new QListWidget;
    disasterFeed_->setObjectName(QStringLiteral("disasterFeedList"));
    disasterFeed_->setAlternatingRowColors(true);
    disasterFeed_->setSpacing(4);
    disasterFeed_->setMinimumWidth(180);
    l1->addWidget(disasterFeed_);
    midRow->addWidget(g1, 1);

    auto *g2 = new QGroupBox(tr("Regional overview"));
    auto *l2 = new QVBoxLayout(g2);
    mapPanel_ = new QLabel;
    mapPanel_->setObjectName(QStringLiteral("mapOverviewPanel"));
    mapPanel_->setWordWrap(true);
    mapPanel_->setMinimumHeight(200);
    mapPanel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    l2->addWidget(mapPanel_, 1);
    midRow->addWidget(g2, 1);

    auto *g3 = new QGroupBox(tr("Emergency requests (queue)"));
    auto *l3 = new QVBoxLayout(g3);
    emergencyFeed_ = new QListWidget;
    emergencyFeed_->setObjectName(QStringLiteral("emergencyFeedList"));
    emergencyFeed_->setAlternatingRowColors(true);
    emergencyFeed_->setMinimumWidth(180);
    l3->addWidget(emergencyFeed_);
    midRow->addWidget(g3, 1);
    root->addLayout(midRow);

    auto *botRow = new QHBoxLayout;
    botRow->setSpacing(16);

    auto *g4 = new QGroupBox(tr("Rescue priority (heap)"));
    auto *l4 = new QVBoxLayout(g4);
    priorityTable_ = new QTableWidget;
    priorityTable_->setColumnCount(4);
    priorityTable_->setHorizontalHeaderLabels({tr("#"), tr("Request"), tr("Location"), tr("Severity")});
    priorityTable_->horizontalHeader()->setStretchLastSection(true);
    priorityTable_->verticalHeader()->setVisible(false);
    priorityTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    priorityTable_->setMaximumHeight(220);
    l4->addWidget(priorityTable_);
    botRow->addWidget(g4, 1);

    auto *g5 = new QGroupBox(tr("Resource allocation (knapsack preview)"));
    auto *l5 = new QVBoxLayout(g5);
    knapsackTable_ = new QTableWidget;
    knapsackTable_->setColumnCount(4);
    knapsackTable_->setHorizontalHeaderLabels({tr("Item"), tr("Weight"), tr("Value"), tr("Selected")});
    knapsackTable_->horizontalHeader()->setStretchLastSection(true);
    knapsackTable_->verticalHeader()->setVisible(false);
    knapsackTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    knapsackTable_->setMaximumHeight(220);
    l5->addWidget(knapsackTable_);
    botRow->addWidget(g5, 1);

    auto *g6 = new QGroupBox(tr("Route optimization (TSP)"));
    auto *l6 = new QVBoxLayout(g6);
    routePanel_ = new QLabel;
    routePanel_->setObjectName(QStringLiteral("routeMiniPanel"));
    routePanel_->setWordWrap(true);
    routePanel_->setMinimumHeight(200);
    routePanel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    l6->addWidget(routePanel_);
    botRow->addWidget(g6, 1);
    root->addLayout(botRow);

    // Footer bar — only real / computed values
    auto *foot = new QFrame;
    foot->setObjectName(QStringLiteral("overviewFooterBar"));
    auto *fl = new QHBoxLayout(foot);
    fl->setContentsMargins(12, 10, 12, 10);
    fl->setSpacing(24);
    footerStatus_ = new QLabel(tr("● All systems operational"));
    footerStatus_->setObjectName(QStringLiteral("footerMetric"));
    footerOps_ = new QLabel;
    footerOps_->setObjectName(QStringLiteral("footerMetric"));
    footerUpdated_ = new QLabel;
    footerUpdated_->setObjectName(QStringLiteral("footerMetricMuted"));
    fl->addWidget(footerStatus_);
    fl->addWidget(footerOps_);
    fl->addStretch();
    fl->addWidget(footerUpdated_);
    root->addWidget(foot);

    scroll->setWidget(body);
    outer->addWidget(scroll, 1);

    refresh();
}

void OverviewPage::refresh()
{
    if (!model_)
        return;

    const QDateTime now = QDateTime::currentDateTime();
    if (dashDateLabel_) {
        dashDateLabel_->setText(now.toString(QStringLiteral("dddd, d MMMM yyyy  |  hh:mm AP")));
    }

    const auto &disasters = model_->disasters.records();
    int sumAffected = 0;
    int sumTeams = 0;
    for (const auto &d : disasters) {
        sumAffected += d.affectedPeople;
        sumTeams += d.teamsNeeded;
    }
    int capTotal = 0;
    int occTotal = 0;
    for (const auto &s : model_->shelters.all()) {
        capTotal += s.capacity;
        occTotal += s.occupied;
    }
    int freeBeds = std::max(0, capTotal - occTotal);
    int pctFree = 0;
    if (capTotal > 0)
        pctFree = static_cast<int>(std::lround(100.0 * static_cast<double>(freeBeds) / static_cast<double>(capTotal)));

    if (kpiDisastersVal_)
        kpiDisastersVal_->setText(QStringLiteral("%1").arg(static_cast<int>(disasters.size()), 2, 10, QLatin1Char('0')));
    if (kpiAffectedVal_)
        kpiAffectedVal_->setText(QString::number(sumAffected));
    if (kpiTeamsVal_)
        kpiTeamsVal_->setText(QString::number(sumTeams));
    if (kpiResourcesVal_)
        kpiResourcesVal_->setText(QStringLiteral("%1 %").arg(pctFree));
    if (kpiSheltersVal_)
        kpiSheltersVal_->setText(QString::number(static_cast<int>(model_->shelters.all().size())));

    if (disasterFeed_) {
        disasterFeed_->clear();
        int n = 0;
        for (const auto &d : disasters) {
            if (++n > 6)
                break;
            const QString line = QStringLiteral("%1 · %2  |  %3 people  |  %4  [%5]")
                                     .arg(QString::fromStdString(d.disasterType),
                                           QString::fromStdString(d.location),
                                           QString::number(d.affectedPeople),
                                           severityLabel(d.severity),
                                           QString::fromStdString(d.status));
            disasterFeed_->addItem(line);
        }
        if (disasterFeed_->count() == 0)
            disasterFeed_->addItem(tr("No incidents registered."));
    }

    if (emergencyFeed_) {
        emergencyFeed_->clear();
        int m = 0;
        for (const auto &e : model_->emergencyQueue.pendingSnapshot()) {
            if (++m > 6)
                break;
            emergencyFeed_->addItem(
                QStringLiteral("%1 — %2")
                    .arg(QString::fromStdString(e.requestType), QString::fromStdString(e.location)));
        }
        if (emergencyFeed_->count() == 0)
            emergencyFeed_->addItem(tr("Queue empty."));
    }

    if (mapPanel_) {
        QString nodes;
        const int nc = model_->roadGraph.nodeCount();
        for (int i = 0; i < nc && i < 8; ++i) {
            if (i)
                nodes += QStringLiteral(" · ");
            nodes += QString::fromStdString(model_->roadGraph.nodeName(i));
        }
        mapPanel_->setText(
            tr("Regional overview — command nodes: %1\n\n"
               "Use Route optimization for Dijkstra / TSP on this graph.")
                .arg(nodes.isEmpty() ? tr("—") : nodes));
    }

    if (priorityTable_) {
        std::vector<PriorityTask> tasks = model_->priorityHeap.snapshotLevels();
        std::sort(tasks.begin(), tasks.end(), [](const PriorityTask &a, const PriorityTask &b) {
            if (a.severity != b.severity)
                return a.severity > b.severity;
            return a.id < b.id;
        });
        const int showN = std::min(5, static_cast<int>(tasks.size()));
        priorityTable_->setRowCount(showN);
        for (int i = 0; i < showN; ++i) {
            const auto &t = tasks[static_cast<std::size_t>(i)];
            priorityTable_->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
            priorityTable_->setItem(
                i, 1, new QTableWidgetItem(QString::fromStdString(t.category)));
            QString loc = QString::fromStdString(t.description);
            if (loc.size() > 48)
                loc = loc.left(45) + QStringLiteral("…");
            priorityTable_->setItem(i, 2, new QTableWidgetItem(loc.isEmpty() ? QStringLiteral("—") : loc));
            priorityTable_->setItem(i, 3, new QTableWidgetItem(severityLabel(t.severity)));
        }
        if (showN == 0) {
            priorityTable_->setRowCount(1);
            for (int c = 0; c < 4; ++c)
                priorityTable_->setItem(0, c, new QTableWidgetItem(
                    c == 1 ? tr("No priority tasks.") : QStringLiteral("—")));
        }
    }

    if (knapsackTable_) {
        auto items = sampleReliefCatalog();
        const int cap = 100;
        KnapsackResult kr = solveReliefKnapsack(items, cap);
        knapsackTable_->setRowCount(static_cast<int>(items.size()));
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            const auto &it = items[static_cast<std::size_t>(i)];
            bool sel = std::find(kr.selectedIndices.begin(), kr.selectedIndices.end(), i) != kr.selectedIndices.end();
            knapsackTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(it.name)));
            knapsackTable_->setItem(i, 1, new QTableWidgetItem(QString::number(it.weight)));
            knapsackTable_->setItem(i, 2, new QTableWidgetItem(QString::number(it.value)));
            knapsackTable_->setItem(i, 3, new QTableWidgetItem(sel ? QStringLiteral("✓") : QStringLiteral("—")));
        }
    }

    if (routePanel_) {
        QString txt;
        if (!model_->tspDistMatrix.empty()) {
            TspResult tspRes = solveTspExact(model_->tspDistMatrix);
            if (tspRes.ok) {
                txt = tr("Approx. convoy tour: %1 distance units\n\n").arg(tspRes.tourCost, 0, 'f', 1);
                txt += tr("Stops: ");
                for (std::size_t i = 0; i < tspRes.tour.size(); ++i) {
                    if (i)
                        txt += QStringLiteral(" → ");
                    int idx = tspRes.tour[i];
                    if (idx >= 0 && idx < static_cast<int>(model_->tspCityLabels.size()))
                        txt += QString::fromStdString(model_->tspCityLabels[static_cast<std::size_t>(idx)]);
                    else
                        txt += QString::number(idx);
                }
                txt += tr("\n\nReturn-to-base included in cost. Open Route optimization for Dijkstra and road blocks.");
            } else {
                txt = tr("TSP data not solvable.");
            }
        } else {
            txt = tr("No district matrix loaded.");
        }
        routePanel_->setText(txt);
    }

    // Footer — only real computed values
    if (footerOps_)
        footerOps_->setText(
            tr("Logged operations: %1").arg(static_cast<int>(model_->history.size())));
    if (footerUpdated_)
        footerUpdated_->setText(tr("Last refresh: %1").arg(now.toString(QStringLiteral("hh:mm:ss"))));
}
