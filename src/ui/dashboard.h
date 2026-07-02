#ifndef DASHBOARD_H
#include <QEvent>
#define DASHBOARD_H

#include "appdata.h"
#include "database.h"
#include "filemanager.h"
#include "thememanager.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Dashboard; }
QT_END_NAMESPACE

class OverviewPage;
class DisasterPage;
class EmergencyPage;
class PriorityPage;
class ResourcePage;
class RoutePage;
class ShelterPage;
class ZonePage;
class ReportsPage;
class AlertsPage;
class UsersPage;
class SettingsPage;
class QFrame;
class QListWidget;
class QLabel;
class QStackedWidget;
class QToolButton;
class QPushButton;
class QTimer;
class QWidget;
class QLineEdit;

class Dashboard : public QMainWindow
{
    Q_OBJECT

public:
    explicit Dashboard(QWidget *parent = nullptr);
    ~Dashboard() override;

    ApplicationModel *model() { return &model_; }
    void setLoggedInUser(const UserRecord &u);
    bool eventFilter(QObject *obj, QEvent *event) override;

    void navigateToPage(int pageIndex);

signals:
    void logoutRequested();

private slots:
    void saveData();
    void loadData();
    void onSidebarToggled(bool expanded);
    void onLiveRefresh();
    void onStackChanged(int index);
    void onSearchChanged(const QString &text);
    void showNotifPanel();
    void showMsgPanel();
    void updateBadges();

private:
    void buildUI();
    void refreshAllPages();
    void applySidebarVisual(bool expanded);

    Ui::Dashboard   *ui;
    ApplicationModel model_;
    FileManager      fileManager_;
    UserRecord       currentUser_;

    OverviewPage  *overview_      = nullptr;
    DisasterPage  *pageDisaster_  = nullptr;
    EmergencyPage *pageEmergency_ = nullptr;
    PriorityPage  *pagePriority_  = nullptr;
    ResourcePage  *pageResource_  = nullptr;
    RoutePage     *pageRoute_     = nullptr;
    ShelterPage   *pageShelter_   = nullptr;
    ZonePage      *pageZone_      = nullptr;
    ReportsPage   *pageReports_   = nullptr;
    AlertsPage    *pageAlerts_    = nullptr;
    UsersPage     *pageUsers_     = nullptr;
    SettingsPage  *pageSettings_  = nullptr;

    QFrame       *navWrap_          = nullptr;
    QWidget      *navInner_         = nullptr;
    QListWidget  *navList_          = nullptr;
    QLabel       *brandTitle_       = nullptr;
    QLabel       *brandSub_         = nullptr;
    QToolButton  *sidebarToggle_    = nullptr;
    QStackedWidget *stack_          = nullptr;
    QLabel       *topClockLabel_    = nullptr;
    QLabel       *contextTitleLabel_ = nullptr;
    QLineEdit    *topSearchField_   = nullptr;
    QLabel       *badgeNotif_       = nullptr;
    QLabel       *badgeMsg_         = nullptr;
    QLabel       *userCapsule_      = nullptr;
    QPushButton  *themeToggleBtn_   = nullptr;
    QTimer       *liveTimer_        = nullptr;

    bool sidebarExpanded_ = true;
    static constexpr int kNavWidthExpanded  = 260;
    static constexpr int kNavWidthCollapsed = 56;
};

#endif // DASHBOARD_H
