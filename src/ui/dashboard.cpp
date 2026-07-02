#include "dashboard.h"
#include "pages/disaster_page.h"
#include "pages/emergency_page.h"
#include "pages/overview_page.h"
#include "pages/priority_page.h"
#include "pages/reports_page.h"
#include "pages/resource_page.h"
#include "pages/route_page.h"
#include "pages/shelter_page.h"
#include "pages/zone_page.h"
#include "pages/alerts_page.h"
#include "pages/users_page.h"
#include "pages/settings_page.h"
#include "ui_dashboard.h"
#include "database.h"
#include "thememanager.h"
#include <QApplication>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

Dashboard::Dashboard(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::Dashboard), fileManager_()
{
    ui->setupUi(this);
    resize(1280, 820);

    QDir().mkpath(model_.dataDirectory());
    fileManager_.setFilePath(
        QDir(model_.dataDirectory()).absoluteFilePath(QStringLiteral("data.txt")));

    buildUI();
    applySidebarVisual(true);

    liveTimer_ = new QTimer(this);
    connect(liveTimer_, &QTimer::timeout, this, &Dashboard::onLiveRefresh);
    // Load persisted refresh interval (default 2s)
    {
        QSettings s;
        const int interval = s.value(QStringLiteral("prefs/refreshInterval"), 2).toInt();
        liveTimer_->start(interval * 1000);
    }
    onLiveRefresh();

    refreshAllPages();
    onStackChanged(0);

    // Connect DB signals
    connect(Database::instance(), &Database::notificationAdded,
            this, &Dashboard::updateBadges);
    connect(Database::instance(), &Database::messageAdded,
            this, &Dashboard::updateBadges);
    connect(Database::instance(), &Database::userAdded,
            this, [this](const UserRecord &) { updateBadges(); if (pageUsers_) pageUsers_->refresh(); });

    // Keep top-bar theme icon in sync with any theme change (e.g. from Settings page)
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](ThemeManager::Theme) {
                if (themeToggleBtn_)
                    themeToggleBtn_->setText(ThemeManager::instance()->isDark()
                                             ? QStringLiteral("☀") : QStringLiteral("🌙"));
            });
}

Dashboard::~Dashboard() { delete ui; }

void Dashboard::setLoggedInUser(const UserRecord &u)
{
    currentUser_ = u;
    const QString display = u.fullName.isEmpty() ? u.username : u.fullName;
    const QString role    = (u.role == QLatin1String("admin")) ? tr("Admin") : tr("User");
    if (userCapsule_)
        userCapsule_->setText(QStringLiteral("  ") + display + QStringLiteral(" · ") + role + QStringLiteral("  "));

    // Role-based access: hide User Management from non-admin users
    if (navList_) {
        const bool isAdmin = (u.role == QLatin1String("admin"));
        // "User management" is at sidebar row 10 (index 10)
        if (navList_->item(10))
            navList_->item(10)->setHidden(!isAdmin);
    }
    updateBadges();
}

void Dashboard::navigateToPage(int pageIndex)
{
    if (stack_ && pageIndex >= 0 && pageIndex < stack_->count())
        stack_->setCurrentIndex(pageIndex);
    if (navList_ && pageIndex >= 0 && pageIndex < navList_->count()) {
        navList_->blockSignals(true);
        navList_->setCurrentRow(pageIndex);
        navList_->blockSignals(false);
    }
}

void Dashboard::buildUI()
{
    auto *central = new QWidget;
    central->setObjectName(QStringLiteral("dashboardRoot"));
    auto *outerLay = new QVBoxLayout(central);
    outerLay->setContentsMargins(0, 0, 0, 0);
    outerLay->setSpacing(0);

    // ── Top bar ───────────────────────────────────────────────────────────────
    auto *topBar = new QFrame;
    topBar->setObjectName(QStringLiteral("appTopBar"));
    auto *topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(20, 10, 20, 10);
    topLay->setSpacing(12);

    sidebarToggle_ = new QToolButton;
    sidebarToggle_->setObjectName(QStringLiteral("sidebarToggle"));
    sidebarToggle_->setCheckable(true);
    sidebarToggle_->setChecked(true);
    sidebarToggle_->setAutoRaise(true);
    sidebarToggle_->setToolButtonStyle(Qt::ToolButtonTextOnly);
    sidebarToggle_->setText(QStringLiteral("☰"));
    sidebarToggle_->setFixedSize(40, 36);

    contextTitleLabel_ = new QLabel(tr("Dashboard"));
    contextTitleLabel_->setObjectName(QStringLiteral("contextPageTitle"));

    topSearchField_ = new QLineEdit;
    topSearchField_->setObjectName(QStringLiteral("topSearchField"));
    topSearchField_->setPlaceholderText(tr("Search modules…"));
    topSearchField_->setClearButtonEnabled(true);
    topSearchField_->setMinimumWidth(220);
    topSearchField_->setMaximumWidth(400);
    connect(topSearchField_, &QLineEdit::textChanged, this, &Dashboard::onSearchChanged);

    // Theme toggle (top-bar canonical control)
    themeToggleBtn_ = new QPushButton(ThemeManager::instance()->isDark() ? QStringLiteral("☀") : QStringLiteral("🌙"));
    themeToggleBtn_->setObjectName(QStringLiteral("btnTheme"));
    themeToggleBtn_->setFixedSize(40, 36);
    themeToggleBtn_->setToolTip(tr("Toggle light / dark mode"));
    connect(themeToggleBtn_, &QPushButton::clicked, this, [this]() {
        ThemeManager::instance()->toggleTheme();
        // icon updated via themeChanged signal connected in constructor
    });

    badgeNotif_ = new QLabel(QStringLiteral("  🔔  0  "));
    badgeNotif_->setObjectName(QStringLiteral("notifBadge"));
    badgeNotif_->setCursor(Qt::PointingHandCursor);
    badgeNotif_->installEventFilter(this);

    badgeMsg_ = new QLabel(QStringLiteral("  ✉  0  "));
    badgeMsg_->setObjectName(QStringLiteral("msgBadge"));
    badgeMsg_->setCursor(Qt::PointingHandCursor);
    badgeMsg_->installEventFilter(this);

    userCapsule_ = new QLabel(QStringLiteral("  Admin · Super Admin  "));
    userCapsule_->setObjectName(QStringLiteral("userCapsule"));
    userCapsule_->setAlignment(Qt::AlignCenter);

    topClockLabel_ = new QLabel;
    topClockLabel_->setObjectName(QStringLiteral("appClockLabel"));
    topClockLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *liveBadge = new QLabel(tr("LIVE"));
    liveBadge->setObjectName(QStringLiteral("liveBadge"));

    topLay->addWidget(sidebarToggle_, 0, Qt::AlignVCenter);
    topLay->addWidget(contextTitleLabel_, 0, Qt::AlignVCenter);
    topLay->addWidget(topSearchField_, 1, Qt::AlignVCenter);
    topLay->addWidget(themeToggleBtn_, 0, Qt::AlignVCenter);
    topLay->addWidget(badgeNotif_, 0, Qt::AlignVCenter);
    topLay->addWidget(badgeMsg_, 0, Qt::AlignVCenter);
    topLay->addWidget(userCapsule_, 0, Qt::AlignVCenter);
    topLay->addWidget(topClockLabel_, 0, Qt::AlignVCenter);
    topLay->addWidget(liveBadge, 0, Qt::AlignVCenter);

    // ── Main layout ───────────────────────────────────────────────────────────
    auto *mainLay = new QHBoxLayout;
    mainLay->setSpacing(0);
    mainLay->setContentsMargins(0, 0, 0, 0);

    // ── Nav sidebar ───────────────────────────────────────────────────────────
    navWrap_ = new QFrame;
    navWrap_->setObjectName(QStringLiteral("navShell"));
    navWrap_->setFixedWidth(kNavWidthExpanded);
    auto *navOuter = new QVBoxLayout(navWrap_);
    navOuter->setContentsMargins(0, 14, 0, 14);
    navOuter->setSpacing(8);

    navInner_ = new QWidget;
    auto *navLay = new QVBoxLayout(navInner_);
    navLay->setContentsMargins(10, 4, 10, 4);
    navLay->setSpacing(14);

    auto *brandRow = new QHBoxLayout;
    brandRow->setSpacing(10);
    brandTitle_ = new QLabel(tr("SDRMS"));
    brandTitle_->setObjectName(QStringLiteral("brandTitle"));
    brandSub_ = new QLabel(tr("Smart Disaster Relief System"));
    brandSub_->setObjectName(QStringLiteral("brandSub"));
    brandSub_->setWordWrap(true);
    auto *brandTextCol = new QVBoxLayout;
    brandTextCol->setSpacing(2);
    brandTextCol->addWidget(brandTitle_);
    brandTextCol->addWidget(brandSub_);
    brandRow->addLayout(brandTextCol, 1);
    navLay->addLayout(brandRow);

    navList_ = new QListWidget;
    navList_->setObjectName(QStringLiteral("navList"));
    navList_->addItem(tr("Dashboard"));
    navList_->addItem(tr("Disaster Management"));
    navList_->addItem(tr("Emergency Requests"));
    navList_->addItem(tr("Rescue Priority (Heap)"));
    navList_->addItem(tr("Resource Allocation"));
    navList_->addItem(tr("Route Optimization"));
    navList_->addItem(tr("Shelter Management"));
    navList_->addItem(tr("Zone Detection"));
    navList_->addItem(tr("Reports & History"));
    navList_->addItem(tr("Alerts & Notifications"));
    navList_->addItem(tr("User Management"));
    navList_->addItem(tr("Settings"));
    navList_->addItem(tr("Logout"));
    navList_->setCurrentRow(0);
    navLay->addWidget(navList_, 1);

    auto *hotline = new QFrame;
    hotline->setObjectName(QStringLiteral("navHotline"));
    auto *hotLay = new QVBoxLayout(hotline);
    hotLay->setContentsMargins(12, 12, 12, 12);
    hotLay->setSpacing(4);
    auto *hotTitle = new QLabel(tr("EMERGENCY HOTLINE"));
    hotTitle->setObjectName(QStringLiteral("hotlineTitle"));
    auto *hotNum = new QLabel(QStringLiteral("1077"));
    hotNum->setObjectName(QStringLiteral("hotlineNumber"));
    auto *hotSub = new QLabel(tr("24/7 Available"));
    hotSub->setObjectName(QStringLiteral("hotlineSub"));
    hotLay->addWidget(hotTitle);
    hotLay->addWidget(hotNum);
    hotLay->addWidget(hotSub);
    navLay->addWidget(hotline);

    navOuter->addWidget(navInner_, 1);

    // ── Content area ──────────────────────────────────────────────────────────
    auto *contentShell = new QFrame;
    contentShell->setObjectName(QStringLiteral("contentShell"));
    auto *contentLay = new QVBoxLayout(contentShell);
    contentLay->setContentsMargins(20, 20, 20, 20);
    contentLay->setSpacing(0);

    stack_ = new QStackedWidget;
    stack_->setObjectName(QStringLiteral("mainStack"));
    stack_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    overview_      = new OverviewPage(&model_, stack_);
    pageDisaster_  = new DisasterPage(&model_, stack_);
    pageEmergency_ = new EmergencyPage(&model_, stack_);
    pagePriority_  = new PriorityPage(&model_, stack_);
    pageResource_  = new ResourcePage(&model_, stack_);
    pageRoute_     = new RoutePage(&model_, stack_);
    pageShelter_   = new ShelterPage(&model_, stack_);
    pageZone_      = new ZonePage(&model_, stack_);
    pageReports_   = new ReportsPage(&model_, stack_);
    pageAlerts_    = new AlertsPage(&model_, stack_);
    pageUsers_     = new UsersPage(stack_);
    pageSettings_  = new SettingsPage(stack_);

    stack_->addWidget(overview_);
    stack_->addWidget(pageDisaster_);
    stack_->addWidget(pageEmergency_);
    stack_->addWidget(pagePriority_);
    stack_->addWidget(pageResource_);
    stack_->addWidget(pageRoute_);
    stack_->addWidget(pageShelter_);
    stack_->addWidget(pageZone_);
    stack_->addWidget(pageReports_);
    stack_->addWidget(pageAlerts_);
    stack_->addWidget(pageUsers_);
    stack_->addWidget(pageSettings_);

    contentLay->addWidget(stack_, 1);
    mainLay->addWidget(navWrap_, 0);
    mainLay->addWidget(contentShell, 1);
    outerLay->addWidget(topBar, 0);
    outerLay->addLayout(mainLay, 1);
    setCentralWidget(central);
    menuBar()->hide();

    // Wire overview KPI tiles to navigate to their respective pages
    connect(overview_, &OverviewPage::navigateRequested, this, &Dashboard::navigateToPage);

    // Wire settings refresh-interval signal to the live timer
    connect(pageSettings_, &SettingsPage::refreshIntervalChanged, this, [this](int secs) {
        if (liveTimer_) {
            liveTimer_->stop();
            liveTimer_->start(secs * 1000);
        }
    });

    // ── Connections ───────────────────────────────────────────────────────────
    const int logoutRow = stack_->count(); // one past the last page
    connect(navList_, &QListWidget::currentRowChanged, this, [this, logoutRow](int row) {
        if (row < 0 || !stack_ || !navList_) return;
        if (row == logoutRow) {
            const int prev = stack_->currentIndex();
            if (QMessageBox::question(this, tr("Logout"),
                                      tr("Log out and return to the login screen?"))
                == QMessageBox::Yes) {
                emit logoutRequested();
                return;
            }
            navList_->blockSignals(true);
            navList_->setCurrentRow(prev);
            navList_->blockSignals(false);
            return;
        }
        if (row < stack_->count()) stack_->setCurrentIndex(row);
    });
    connect(stack_, &QStackedWidget::currentChanged, this, &Dashboard::onStackChanged);
    connect(sidebarToggle_, &QToolButton::toggled, this, &Dashboard::onSidebarToggled);

    auto *scSave = new QShortcut(QKeySequence::Save, this);
    connect(scSave, &QShortcut::activated, this, &Dashboard::saveData);
    auto *scOpen = new QShortcut(QKeySequence::Open, this);
    connect(scOpen, &QShortcut::activated, this, &Dashboard::loadData);
}

bool Dashboard::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == badgeNotif_) { showNotifPanel(); return true; }
        if (obj == badgeMsg_)   { showMsgPanel();   return true; }
    }
    return QMainWindow::eventFilter(obj, event);
}

void Dashboard::updateBadges()
{
    int nc = Database::instance()->unreadNotifCount();
    int mc = Database::instance()->unreadMsgCount();

    if (badgeNotif_) {
        badgeNotif_->setText(QStringLiteral("  🔔  ") + QString::number(nc) + QStringLiteral("  "));
        badgeNotif_->setProperty("unread", nc > 0);
        badgeNotif_->style()->unpolish(badgeNotif_);
        badgeNotif_->style()->polish(badgeNotif_);
    }
    if (badgeMsg_) {
        badgeMsg_->setText(QStringLiteral("  ✉  ") + QString::number(mc) + QStringLiteral("  "));
        badgeMsg_->setProperty("unread", mc > 0);
        badgeMsg_->style()->unpolish(badgeMsg_);
        badgeMsg_->style()->polish(badgeMsg_);
    }
}

void Dashboard::showNotifPanel()
{
    auto *dlg = new QDialog(this);
    dlg->setObjectName(QStringLiteral("notifPanel"));
    dlg->setWindowTitle(tr("Notifications"));
    dlg->resize(480, 420);
    auto *lay = new QVBoxLayout(dlg);
    lay->setContentsMargins(16, 16, 16, 16);
    lay->setSpacing(10);

    auto *header = new QHBoxLayout;
    auto *title  = new QLabel(tr("🔔  Notifications"));
    title->setObjectName(QStringLiteral("panelTitle"));
    auto *markAll = new QPushButton(tr("Mark all read"));
    markAll->setObjectName(QStringLiteral("btnGhost"));
    header->addWidget(title, 1);
    header->addWidget(markAll);
    lay->addLayout(header);

    auto *list = new QListWidget;
    list->setObjectName(QStringLiteral("notifList"));
    lay->addWidget(list, 1);

    auto reload = [list]() {
        list->clear();
        for (const auto &n : Database::instance()->notifications()) {
            const QString prefix = n.isRead ? QStringLiteral("   ") : QStringLiteral("🔵 ");
            list->addItem(prefix + QStringLiteral("[") + n.type + QStringLiteral("] ")
                          + n.title + QStringLiteral("\n   ") + n.message
                          + QStringLiteral("  (") + n.createdAt + QStringLiteral(")"));
        }
        if (list->count() == 0)
            list->addItem(tr("No notifications yet."));
    };
    reload();

    connect(markAll, &QPushButton::clicked, dlg, [=]() {
        Database::instance()->markAllNotifsRead();
        reload();
    });
    connect(dlg, &QDialog::finished, this, &Dashboard::updateBadges);

    dlg->exec();
}

void Dashboard::showMsgPanel()
{
    auto *dlg = new QDialog(this);
    dlg->setObjectName(QStringLiteral("msgPanel"));
    dlg->setWindowTitle(tr("Messages"));
    dlg->resize(500, 440);
    auto *lay = new QVBoxLayout(dlg);
    lay->setContentsMargins(16, 16, 16, 16);
    lay->setSpacing(10);

    auto *header = new QHBoxLayout;
    auto *title = new QLabel(tr("✉  Disaster Report Messages"));
    title->setObjectName(QStringLiteral("panelTitle"));
    auto *markAll = new QPushButton(tr("Mark all read"));
    markAll->setObjectName(QStringLiteral("btnGhost"));
    header->addWidget(title, 1);
    header->addWidget(markAll);
    lay->addLayout(header);

    auto *list = new QListWidget;
    list->setObjectName(QStringLiteral("msgList"));
    lay->addWidget(list, 1);

    auto reload = [list]() {
        list->clear();
        for (const auto &m : Database::instance()->messages()) {
            const QString prefix = m.isRead ? QStringLiteral("   ") : QStringLiteral("📩 ");
            list->addItem(prefix + tr("From: ") + m.sender
                          + QStringLiteral("  |  ") + m.subject
                          + QStringLiteral("\n   ") + m.body
                          + QStringLiteral("  (") + m.createdAt + QStringLiteral(")"));
        }
        if (list->count() == 0)
            list->addItem(tr("No messages yet."));
    };
    reload();

    connect(markAll, &QPushButton::clicked, dlg, [=]() {
        // Mark all messages as read
        for (const auto &m : Database::instance()->messages())
            Database::instance()->markMsgRead(m.id);
        reload();
    });

    connect(list, &QListWidget::itemClicked, dlg, [=](QListWidgetItem *) {
        auto msgs = Database::instance()->messages();
        int row = list->currentRow();
        if (row >= 0 && row < msgs.size())
            Database::instance()->markMsgRead(msgs[row].id);
        reload();
    });
    connect(dlg, &QDialog::finished, this, &Dashboard::updateBadges);
    dlg->exec();
}

void Dashboard::onSearchChanged(const QString &text)
{
    if (!navList_) return;
    const QString t = text.trimmed().toLower();
    for (int i = 0; i < navList_->count(); ++i) {
        auto *item = navList_->item(i);
        if (item) item->setHidden(!t.isEmpty() && !item->text().toLower().contains(t));
    }
}

void Dashboard::saveData()
{
    if (fileManager_.saveAll(model_))
        statusBar()->showMessage(tr("Saved successfully."), 4000);
    else
        QMessageBox::warning(this, tr("Save failed"), tr("Could not write data file."));
}

void Dashboard::loadData()
{
    if (!QFile::exists(fileManager_.filePath())) {
        statusBar()->showMessage(tr("No save file found."), 3000); return;
    }
    if (fileManager_.loadAll(model_)) {
        refreshAllPages();
        statusBar()->showMessage(tr("Data loaded."), 3000);
    }
}

void Dashboard::refreshAllPages()
{
    if (overview_)      overview_->refresh();
    if (pageDisaster_)  pageDisaster_->refresh();
    if (pageEmergency_) pageEmergency_->refresh();
    if (pagePriority_)  pagePriority_->refresh();
    if (pageResource_)  pageResource_->refresh();
    if (pageRoute_)     pageRoute_->refresh();
    if (pageShelter_)   pageShelter_->refresh();
    if (pageZone_)      pageZone_->refresh();
    if (pageReports_)   pageReports_->refresh();
    if (pageAlerts_)    pageAlerts_->refresh();
    if (pageUsers_)     pageUsers_->refresh();
    statusBar()->showMessage(tr("Workspace synchronized."), 2500);
}

void Dashboard::applySidebarVisual(bool expanded)
{
    sidebarExpanded_ = expanded;
    if (!navWrap_ || !navInner_) return;
    navWrap_->setFixedWidth(expanded ? kNavWidthExpanded : kNavWidthCollapsed);
    navInner_->setVisible(expanded);
}

void Dashboard::onSidebarToggled(bool expanded) { applySidebarVisual(expanded); }

void Dashboard::onLiveRefresh()
{
    if (topClockLabel_)
        topClockLabel_->setText(
            QDateTime::currentDateTime().toString(QStringLiteral("ddd d MMM  hh:mm:ss")));
    if (overview_) overview_->refresh();
    updateBadges();
}

void Dashboard::onStackChanged(int index)
{
    const QStringList titles = {tr("Dashboard"), tr("Disaster management"),
        tr("Emergency requests"), tr("Rescue priority"), tr("Resource allocation"),
        tr("Route optimization"), tr("Shelter management"), tr("Zone detection"),
        tr("Reports & history"), tr("Alerts & notifications"),
        tr("User management"), tr("Settings")};
    if (contextTitleLabel_ && index >= 0 && index < titles.size()) {
        QFontMetrics fm(contextTitleLabel_->font());
        contextTitleLabel_->setText(fm.elidedText(titles.at(index), Qt::ElideRight, 230));
        contextTitleLabel_->setToolTip(titles.at(index));
    }
    if (navList_ && stack_ && index >= 0 && index < stack_->count()) {
        navList_->blockSignals(true);
        if (navList_->currentRow() != index) navList_->setCurrentRow(index);
        navList_->blockSignals(false);
    }
    if (index == 0 && overview_) overview_->refresh();
}
