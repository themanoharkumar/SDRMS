#include "users_page.h"
#include "page_header.h"
#include "page_scroll.h"
#include "database.h"
#include "thememanager.h"
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

UsersPage::UsersPage(QWidget *parent) : QWidget(parent)
{
    auto *root = pageScrollSetup(this);
    addSdrmsPanelHeader(root, 12, tr("User management"),
                        tr("All registered users — manage accounts and permissions."));

    // ── Stats bar ─────────────────────────────────────────────────────────────
    auto *statsBar = new QHBoxLayout;
    countLabel_ = new QLabel;
    countLabel_->setObjectName(QStringLiteral("pageSubtitle"));
    statsBar->addWidget(countLabel_);
    statsBar->addStretch();
    root->addLayout(statsBar);

    // ── Table ─────────────────────────────────────────────────────────────────
    table_ = new QTableWidget;
    table_->setColumnCount(7);
    table_->setHorizontalHeaderLabels(
        {tr("ID"), tr("Username"), tr("Full name"), tr("Email"),
         tr("Role"), tr("Joined"), tr("Action")});
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table_->verticalHeader()->setVisible(false);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    root->addWidget(table_, 1);

    refresh();
}

void UsersPage::refresh()
{
    if (!table_) return;
    const auto users = Database::instance()->allUsers();
    table_->setRowCount(users.size());

    if (countLabel_)
        countLabel_->setText(tr("Total users: %1").arg(users.size()));

    for (int i = 0; i < users.size(); ++i) {
        const auto &u = users[i];

        auto mkItem = [](const QString &t) {
            auto *it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
            return it;
        };

        table_->setItem(i, 0, mkItem(QString::number(u.id)));
        table_->setItem(i, 1, mkItem(u.username));
        table_->setItem(i, 2, mkItem(u.fullName));
        table_->setItem(i, 3, mkItem(u.email));

        auto *roleItem = mkItem(u.role == QLatin1String("admin") ? tr("Admin") : tr("User"));
        bool isDark = ThemeManager::instance()->isDark();
        if (u.role == QLatin1String("admin")) {
            roleItem->setForeground(isDark ? QColor(QStringLiteral("#58a6ff"))
                                           : QColor(QStringLiteral("#0969da")));
        } else {
            roleItem->setForeground(isDark ? QColor(QStringLiteral("#3fb950"))
                                           : QColor(QStringLiteral("#1a7f37")));
        }
        table_->setItem(i, 4, roleItem);
        table_->setItem(i, 5, mkItem(u.createdAt.left(10)));

        // Action button (disable/enable — skip admin)
        if (u.username == QLatin1String("admin")) {
            table_->setItem(i, 6, mkItem(tr("Protected")));
        } else {
            auto *btn = new QPushButton(u.isActive ? tr("Disable") : tr("Enable"));
            btn->setObjectName(u.isActive ? QStringLiteral("btnDanger") : QStringLiteral("btnSuccess"));
            const int uid = u.id;
            const bool active = u.isActive;
            connect(btn, &QPushButton::clicked, this, [this, uid, active]() {
                toggleUserActive(uid, active);
            });
            table_->setCellWidget(i, 6, btn);
        }
        table_->setRowHeight(i, 44);
    }
    table_->resizeColumnToContents(0);
    table_->resizeColumnToContents(4);
    table_->resizeColumnToContents(5);
    table_->setColumnWidth(6, 100);
}

void UsersPage::toggleUserActive(int id, bool currentlyActive)
{
    const QString action = currentlyActive ? tr("disable") : tr("enable");
    if (QMessageBox::question(this, tr("Confirm"),
                              tr("Are you sure you want to %1 this user?").arg(action))
        != QMessageBox::Yes)
        return;
    Database::instance()->setUserActive(id, !currentlyActive);
    refresh();
}
