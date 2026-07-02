#include "disaster_page.h"
#include "page_header.h"
#include "page_scroll.h"
#include "appdata.h"
#include "database.h"
#include <QComboBox>
#include <QDateTime>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHBoxLayout>
#include <QVBoxLayout>

// Status cycle order
static const QStringList kStatusCycle = {"Active", "Monitoring", "Resolved"};

DisasterPage::DisasterPage(ApplicationModel *model, QWidget *parent)
    : QWidget(parent), model_(model)
{
    auto *root = pageScrollSetup(this);
    addSdrmsPanelHeader(root, 3, tr("Disaster registration"),
                        tr("Register new disasters. Reports are sent to admin messages."));

    auto *form = new QGroupBox(tr("Register new disaster"));
    auto *gl   = new QGridLayout(form);
    gl->setVerticalSpacing(10);
    gl->setHorizontalSpacing(14);

    type_ = new QComboBox;
    type_->addItems({tr("Flood"), tr("Earthquake"), tr("Fire"), tr("Cyclone"),
                     tr("Landslide"), tr("Industrial hazard"), tr("Other")});
    location_ = new QLineEdit;
    location_->setPlaceholderText(tr("e.g. East Riverside"));
    severity_ = new QSpinBox;
    severity_->setRange(1, 10); severity_->setValue(5);
    affected_ = new QSpinBox;
    affected_->setRange(0, 10000000); affected_->setValue(100);
    teams_ = new QSpinBox;
    teams_->setRange(0, 10000); teams_->setValue(5);

    int r = 0;
    gl->addWidget(new QLabel(tr("Type")),               r, 0); gl->addWidget(type_,     r, 1); ++r;
    gl->addWidget(new QLabel(tr("Location")),            r, 0); gl->addWidget(location_, r, 1); ++r;
    gl->addWidget(new QLabel(tr("Severity (1–10)")),     r, 0); gl->addWidget(severity_, r, 1); ++r;
    gl->addWidget(new QLabel(tr("Affected people")),     r, 0); gl->addWidget(affected_, r, 1); ++r;
    gl->addWidget(new QLabel(tr("Rescue teams needed")), r, 0); gl->addWidget(teams_,    r, 1); ++r;

    auto *btnAdd = new QPushButton(tr("Register disaster"));
    gl->addWidget(btnAdd, r, 0, 1, 2);
    connect(btnAdd, &QPushButton::clicked, this, &DisasterPage::addDisaster);

    // Table: Type | Location | Severity | Affected | Teams | Status | Registered | [Status Btn] | [Delete Btn]
    table_ = new QTableWidget;
    table_->setColumnCount(9);
    table_->setHorizontalHeaderLabels(
        {tr("Type"), tr("Location"), tr("Severity"), tr("Affected"), tr("Teams"),
         tr("Status"), tr("Registered"), tr("Change status"), tr("Delete")});
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    table_->setAlternatingRowColors(true);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    auto *allBox = new QGroupBox(tr("All disasters"));
    auto *allLay = new QVBoxLayout(allBox);
    allLay->addWidget(table_);

    auto *split = new QHBoxLayout;
    split->addWidget(form, 2);
    split->addWidget(allBox, 3);
    root->addLayout(split, 1);

    refresh();
}

void DisasterPage::rebuildTable()
{
    const auto &rows = model_->disasters.records();
    table_->setRowCount(static_cast<int>(rows.size()));
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const auto &d = rows[static_cast<std::size_t>(i)];
        table_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(d.disasterType)));
        table_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(d.location)));
        table_->setItem(i, 2, new QTableWidgetItem(QString::number(d.severity)));
        table_->setItem(i, 3, new QTableWidgetItem(QString::number(d.affectedPeople)));
        table_->setItem(i, 4, new QTableWidgetItem(QString::number(d.teamsNeeded)));

        // Status cell with color coding
        auto *statusItem = new QTableWidgetItem(QString::fromStdString(d.status));
        if (d.status == "Active")
            statusItem->setForeground(QColor("#dc2626"));
        else if (d.status == "Monitoring")
            statusItem->setForeground(QColor("#d97706"));
        else
            statusItem->setForeground(QColor("#059669"));
        table_->setItem(i, 5, statusItem);

        // Timestamp
        table_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(d.timestamp)));

        // Cycle-status button
        auto *btnStatus = new QPushButton(tr("Next status"));
        btnStatus->setObjectName(QStringLiteral("btnGhost"));
        const int row = i;
        connect(btnStatus, &QPushButton::clicked, this, [this, row]() { cycleStatus(row); });
        table_->setCellWidget(i, 7, btnStatus);

        // Delete button
        auto *btnDel = new QPushButton(tr("Delete"));
        btnDel->setObjectName(QStringLiteral("btnDanger"));
        connect(btnDel, &QPushButton::clicked, this, [this, row]() { deleteRow(row); });
        table_->setCellWidget(i, 8, btnDel);

        table_->setRowHeight(i, 44);
    }
    table_->resizeColumnToContents(0);
    table_->resizeColumnToContents(2);
    table_->resizeColumnToContents(3);
    table_->resizeColumnToContents(4);
    table_->resizeColumnToContents(5);
    table_->setColumnWidth(7, 110);
    table_->setColumnWidth(8, 80);
}

void DisasterPage::refresh() { rebuildTable(); }

void DisasterPage::addDisaster()
{
    if (location_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Please enter a location."));
        return;
    }
    DisasterRecord d;
    d.disasterType   = type_->currentText().toStdString();
    d.location       = location_->text().trimmed().toStdString();
    d.severity       = severity_->value();
    d.affectedPeople = affected_->value();
    d.teamsNeeded    = teams_->value();
    d.status         = "Active";
    d.timestamp      = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
    model_->disasters.registerDisaster(d);
    refresh();

    // Push notification + message to admin
    const QString loc  = location_->text().trimmed();
    const QString type = type_->currentText();
    Database::instance()->pushNotification(
        QStringLiteral("Disaster reported"),
        QStringLiteral("%1 at %2 — severity %3").arg(type, loc, QString::number(severity_->value())),
        QStringLiteral("disaster"));
    Database::instance()->pushMessage(
        QStringLiteral("Disaster System"),
        QStringLiteral("New report: ") + type + QStringLiteral(" @ ") + loc,
        QStringLiteral("Type: %1\nLocation: %2\nSeverity: %3/10\nAffected: %4\nTeams needed: %5\nRegistered: %6")
            .arg(type, loc, QString::number(severity_->value()),
                 QString::number(affected_->value()), QString::number(teams_->value()),
                 QDateTime::currentDateTime().toString(Qt::ISODate)));

    QMessageBox::information(this, tr("Saved"),
                             tr("Disaster registered and admin notified."));
}

void DisasterPage::deleteRow(int row)
{
    if (row < 0 || row >= static_cast<int>(model_->disasters.records().size()))
        return;
    const auto &d = model_->disasters.records()[static_cast<std::size_t>(row)];
    const QString msg = tr("Delete disaster record:\n%1 at %2?")
                            .arg(QString::fromStdString(d.disasterType),
                                 QString::fromStdString(d.location));
    if (QMessageBox::question(this, tr("Confirm delete"), msg) != QMessageBox::Yes)
        return;
    model_->disasters.removeAt(row);
    refresh();
}

void DisasterPage::cycleStatus(int row)
{
    auto &records = model_->disasters.records();
    if (row < 0 || row >= static_cast<int>(records.size()))
        return;
    auto &d = records[static_cast<std::size_t>(row)];
    const QString current = QString::fromStdString(d.status);
    int idx = kStatusCycle.indexOf(current);
    idx = (idx + 1) % kStatusCycle.size();
    d.status = kStatusCycle[idx].toStdString();
    refresh();
}
