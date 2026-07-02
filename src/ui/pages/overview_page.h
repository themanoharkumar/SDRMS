#ifndef OVERVIEW_PAGE_H
#define OVERVIEW_PAGE_H

#include <QWidget>

class ApplicationModel;
class QLabel;
class QListWidget;
class QTableWidget;

/**
 * Executive dashboard: KPI strip, feeds, map placeholder, DSA preview panels.
 * KPI tiles emit navigateRequested(pageIndex) when clicked.
 */
class OverviewPage : public QWidget
{
    Q_OBJECT
public:
    explicit OverviewPage(ApplicationModel *model, QWidget *parent = nullptr);
    void refresh();

signals:
    /** Emitted when a KPI tile "View details" link is clicked.
     *  pageIndex matches the stacked-widget page order in Dashboard. */
    void navigateRequested(int pageIndex);

private:
    ApplicationModel *model_;

    QLabel *dashDateLabel_ = nullptr;
    QLabel *kpiDisastersVal_ = nullptr;
    QLabel *kpiAffectedVal_ = nullptr;
    QLabel *kpiTeamsVal_ = nullptr;
    QLabel *kpiResourcesVal_ = nullptr;
    QLabel *kpiSheltersVal_ = nullptr;

    QListWidget *disasterFeed_ = nullptr;
    QListWidget *emergencyFeed_ = nullptr;
    QLabel *mapPanel_ = nullptr;

    QTableWidget *priorityTable_ = nullptr;
    QTableWidget *knapsackTable_ = nullptr;
    QLabel *routePanel_ = nullptr;

    QLabel *footerStatus_  = nullptr;
    QLabel *footerOps_     = nullptr;
    QLabel *footerUpdated_ = nullptr;
};

#endif // OVERVIEW_PAGE_H
