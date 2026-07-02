#include "shelter_page.h"
#include <algorithm>
#include "page_header.h"
#include "page_scroll.h"
#include "appdata.h"
#include <QDateTime>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHBoxLayout>
#include <QVBoxLayout>

ShelterPage::ShelterPage(ApplicationModel *model, QWidget *parent)
    : QWidget(parent)
    , model_(model)
{
    auto *root = pageScrollSetup(this);
    addSdrmsPanelHeader(root, 8, tr("Shelter management"),
                        tr("Capacity, occupancy, and assignments to the best available facility."));

    auto *form = new QGroupBox(tr("Add shelter"));
    auto *gl = new QGridLayout(form);
    name_ = new QLineEdit;
    loc_  = new QLineEdit;
    cap_  = new QSpinBox;
    cap_->setRange(1, 1000000);
    cap_->setValue(400);
    occ_ = new QSpinBox;
    occ_->setRange(0, 1000000);
    occ_->setValue(0);

    int r = 0;
    gl->addWidget(new QLabel(tr("Name")),             r, 0); gl->addWidget(name_, r, 1); ++r;
    gl->addWidget(new QLabel(tr("Location")),         r, 0); gl->addWidget(loc_,  r, 1); ++r;
    gl->addWidget(new QLabel(tr("Capacity")),         r, 0); gl->addWidget(cap_,  r, 1); ++r;
    gl->addWidget(new QLabel(tr("Current occupancy")), r, 0); gl->addWidget(occ_,  r, 1);
    auto *btn = new QPushButton(tr("Add shelter"));
    connect(btn, &QPushButton::clicked, this, &ShelterPage::addShelter);
    gl->addWidget(btn, ++r, 1);

    auto *assignBox = new QGroupBox(tr("Assign beds (nearest by available capacity)"));
    auto *al = new QGridLayout(assignBox);
    assignIdx_ = new QSpinBox;
    assignIdx_->setRange(0, 9999);
    assignBeds_ = new QSpinBox;
    assignBeds_->setRange(1, 10000);
    assignBeds_->setValue(10);
    al->addWidget(new QLabel(tr("Shelter row index (0-based)")), 0, 0);
    al->addWidget(assignIdx_, 0, 1);
    al->addWidget(new QLabel(tr("Beds to allocate")), 1, 0);
    al->addWidget(assignBeds_, 1, 1);
    auto *btn2 = new QPushButton(tr("Auto-pick shelter with most free beds"));
    connect(btn2, &QPushButton::clicked, this, [this]() {
        int idx = model_->shelters.nearestByCapacity("");
        if (idx < 0) {
            QMessageBox::information(this, tr("Shelters"), tr("No shelters registered."));
            return;
        }
        assignIdx_->setValue(idx);
    });
    al->addWidget(btn2, 2, 1);
    auto *btn3 = new QPushButton(tr("Confirm assignment"));
    connect(btn3, &QPushButton::clicked, this, &ShelterPage::assignBeds);
    al->addWidget(btn3, 3, 1);

    auto *leftCol = new QVBoxLayout;
    leftCol->addWidget(form);
    leftCol->addWidget(assignBox);
    leftCol->addStretch();

    // Table: Name | Location | Capacity | Occupied | Available | Capacity Bar | Delete
    table_ = new QTableWidget;
    table_->setColumnCount(7);
    table_->setHorizontalHeaderLabels(
        {tr("Name"), tr("Location"), tr("Capacity"), tr("Occupied"), tr("Available"), tr("Utilization"), tr("Delete")});
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    table_->setAlternatingRowColors(true);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    auto *listBox = new QGroupBox(tr("Shelter list"));
    auto *listLay = new QVBoxLayout(listBox);
    listLay->addWidget(table_);

    auto *split = new QHBoxLayout;
    split->addLayout(leftCol, 2);
    split->addWidget(listBox, 3);
    root->addLayout(split, 1);

    refresh();
}

void ShelterPage::rebuildTable()
{
    const auto &rows = model_->shelters.all();
    table_->setRowCount(static_cast<int>(rows.size()));
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const auto &s = rows[static_cast<std::size_t>(i)];
        const int avail = std::max(0, s.capacity - s.occupied);
        const int pct   = s.capacity > 0
                              ? std::min(100, (s.occupied * 100) / s.capacity)
                              : 0;

        table_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(s.name)));
        table_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(s.location)));
        table_->setItem(i, 2, new QTableWidgetItem(QString::number(s.capacity)));
        table_->setItem(i, 3, new QTableWidgetItem(QString::number(s.occupied)));
        table_->setItem(i, 4, new QTableWidgetItem(QString::number(avail)));

        // Visual capacity progress bar
        auto *bar = new QProgressBar;
        bar->setRange(0, 100);
        bar->setValue(pct);
        bar->setFormat(QStringLiteral("%p%"));
        if (pct >= 90)
            bar->setStyleSheet(QStringLiteral("QProgressBar::chunk { background: #dc2626; }"));
        else if (pct >= 70)
            bar->setStyleSheet(QStringLiteral("QProgressBar::chunk { background: #d97706; }"));
        else
            bar->setStyleSheet(QStringLiteral("QProgressBar::chunk { background: #059669; }"));
        table_->setCellWidget(i, 5, bar);

        // Delete button
        auto *btnDel = new QPushButton(tr("Delete"));
        btnDel->setObjectName(QStringLiteral("btnDanger"));
        const int row = i;
        connect(btnDel, &QPushButton::clicked, this, [this, row]() { deleteRow(row); });
        table_->setCellWidget(i, 6, btnDel);

        table_->setRowHeight(i, 44);
    }
    table_->setColumnWidth(6, 80);
}

void ShelterPage::refresh()
{
    rebuildTable();
}

void ShelterPage::addShelter()
{
    if (name_->text().trimmed().isEmpty() || loc_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Name and location are required."));
        return;
    }
    if (occ_->value() > cap_->value()) {
        QMessageBox::warning(this, tr("Validation"), tr("Occupancy cannot exceed capacity."));
        return;
    }
    ShelterRecord s;
    s.name     = name_->text().trimmed().toStdString();
    s.location = loc_->text().trimmed().toStdString();
    s.capacity = cap_->value();
    s.occupied = occ_->value();
    model_->shelters.addShelter(s);
    model_->history.push({QDateTime::currentDateTime().toString(Qt::ISODate).toStdString(),
                          "Shelter", "Shelter registered", s.name + " @ " + s.location});
    refresh();
}

void ShelterPage::assignBeds()
{
    int idx  = assignIdx_->value();
    int beds = assignBeds_->value();
    if (!model_->shelters.assignBeds(idx, beds)) {
        QMessageBox::warning(this, tr("Assignment"),
                             tr("Could not assign — check index, capacity, and values."));
        return;
    }
    model_->history.push({QDateTime::currentDateTime().toString(Qt::ISODate).toStdString(),
                          "Shelter", "Beds assigned",
                          "shelterIndex=" + std::to_string(idx) + " beds=" + std::to_string(beds)});
    refresh();
    QMessageBox::information(this, tr("Shelter"), tr("Assignment successful."));
}

void ShelterPage::deleteRow(int row)
{
    const auto &shelters = model_->shelters.all();
    if (row < 0 || row >= static_cast<int>(shelters.size()))
        return;
    const QString msg = tr("Remove shelter \"%1\" at %2?")
                            .arg(QString::fromStdString(shelters[static_cast<std::size_t>(row)].name),
                                 QString::fromStdString(shelters[static_cast<std::size_t>(row)].location));
    if (QMessageBox::question(this, tr("Confirm delete"), msg) != QMessageBox::Yes)
        return;
    model_->shelters.removeAt(row);
    model_->history.push({QDateTime::currentDateTime().toString(Qt::ISODate).toStdString(),
                          "Shelter", "Shelter removed",
                          "index=" + std::to_string(row)});
    refresh();
}
