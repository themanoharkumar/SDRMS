#ifndef ALERTS_PAGE_H
#define ALERTS_PAGE_H

#include <QWidget>

class ApplicationModel;
class QListWidget;

class AlertsPage : public QWidget
{
    Q_OBJECT
public:
    explicit AlertsPage(ApplicationModel *model, QWidget *parent = nullptr);
    void refresh();

private:
    ApplicationModel *model_;
    QListWidget *list_     = nullptr;  // DB notifications
    QListWidget *histList_ = nullptr;  // local history stack
};

#endif // ALERTS_PAGE_H
