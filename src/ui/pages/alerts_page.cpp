#include "alerts_page.h"
#include "page_header.h"
#include "page_scroll.h"
#include "appdata.h"
#include "database.h"
#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

AlertsPage::AlertsPage(ApplicationModel *model, QWidget *parent)
    : QWidget(parent), model_(model)
{
    auto *root = pageScrollSetup(this);
    addSdrmsPanelHeader(root, 11, tr("Alerts & notifications"),
                        tr("Live notifications from user registrations and disaster reports."));

    auto *split = new QHBoxLayout;

    // ── Active alerts (DB notifications only) ─────────────────────────────────
    auto *g1  = new QGroupBox(tr("Active alerts (database)"));
    auto *l1  = new QVBoxLayout(g1);

    auto *btnBar = new QHBoxLayout;
    auto *btnMarkAll = new QPushButton(tr("Mark all read"));
    btnMarkAll->setObjectName(QStringLiteral("btnGhost"));
    btnBar->addStretch();
    btnBar->addWidget(btnMarkAll);
    l1->addLayout(btnBar);

    list_ = new QListWidget;
    list_->setObjectName(QStringLiteral("notifList"));
    list_->setAlternatingRowColors(true);
    l1->addWidget(list_);

    connect(btnMarkAll, &QPushButton::clicked, this, [this]() {
        Database::instance()->markAllNotifsRead();
        refresh();
    });
    connect(list_, &QListWidget::itemClicked, this, [this](QListWidgetItem *) {
        auto notifs = Database::instance()->notifications();
        int row = list_->currentRow();
        if (row >= 0 && row < notifs.size())
            Database::instance()->markNotifRead(notifs[row].id);
        refresh();
    });

    // ── Operation history (local stack — separate section) ────────────────────
    auto *g2  = new QGroupBox(tr("Operation history (local stack)"));
    auto *l2  = new QVBoxLayout(g2);
    histList_ = new QListWidget;
    histList_->setObjectName(QStringLiteral("historyList"));
    histList_->setAlternatingRowColors(true);
    l2->addWidget(histList_);

    split->addWidget(g1, 3);
    split->addWidget(g2, 2);
    root->addLayout(split, 1);

    // ── Legend panel ──────────────────────────────────────────────────────────
    auto *legendBox = new QGroupBox(tr("Notification guide"));
    auto *ll = new QVBoxLayout(legendBox);
    ll->addWidget(new QLabel(tr("🔵  Blue dot = unread notification")));
    ll->addWidget(new QLabel(tr("👤  user_signup = new user registered")));
    ll->addWidget(new QLabel(tr("🌪  disaster = disaster report submitted")));
    ll->addWidget(new QLabel(tr("⚙  system = system event")));
    ll->addWidget(new QLabel(tr("\nAdmin gets a notification when:\n"
                                "• A new user signs up\n"
                                "• A user submits a disaster report")));
    ll->addStretch();
    root->addWidget(legendBox);

    // Connect for live updates
    connect(Database::instance(), &Database::notificationAdded,
            this, &AlertsPage::refresh);

    refresh();
}

void AlertsPage::refresh()
{
    // ── DB notifications ──────────────────────────────────────────────────────
    if (list_) {
        list_->clear();
        const auto notifs = Database::instance()->notifications();
        for (const auto &n : notifs) {
            const QString prefix = n.isRead ? QStringLiteral("   ") : QStringLiteral("🔵 ");
            list_->addItem(prefix + QStringLiteral("[") + n.type + QStringLiteral("]  ")
                           + n.title + QStringLiteral("\n        ") + n.message
                           + QStringLiteral("   ") + n.createdAt);
        }
        if (list_->count() == 0)
            list_->addItem(tr("No alerts yet."));
    }

    // ── Local operation history stack (separate list) ─────────────────────────
    if (histList_ && model_) {
        histList_->clear();
        const auto entries = model_->history.toVectorOldestFirst();
        // Show newest first
        for (int i = static_cast<int>(entries.size()) - 1; i >= 0; --i) {
            const auto &h = entries[static_cast<std::size_t>(i)];
            histList_->addItem(QStringLiteral("[") + QString::fromStdString(h.operationType)
                               + QStringLiteral("]  ") + QString::fromStdString(h.summary)
                               + QStringLiteral("  ") + QString::fromStdString(h.timestamp));
        }
        if (histList_->count() == 0)
            histList_->addItem(tr("No operations recorded yet."));
    }
}
