#include "resource_page.h"
#include "page_header.h"
#include "page_scroll.h"
#include "appdata.h"
#include "knapsack.h"
#include <QAbstractItemView>
#include <QDateTime>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>
#include <algorithm>

ResourcePage::ResourcePage(ApplicationModel *model, QWidget *parent)
    : QWidget(parent)
    , model_(model)
{
    auto *root = pageScrollSetup(this);
    addSdrmsPanelHeader(root, 6, tr("Resource allocation (Knapsack)"),
                        tr("0/1 Knapsack — maximize relief value under convoy capacity (weight units)."));

    auto *top = new QGroupBox(tr("Transport capacity"));
    auto *hl = new QHBoxLayout(top);
    hl->addWidget(new QLabel(tr("Capacity (weight units)")));
    capacity_ = new QSpinBox;
    capacity_->setRange(1, 100000);
    capacity_->setValue(120);
    hl->addWidget(capacity_);
    auto *btn = new QPushButton(tr("Compute optimal loadout"));
    connect(btn, &QPushButton::clicked, this, &ResourcePage::runKnapsack);
    hl->addWidget(btn);
    hl->addStretch();
    root->addWidget(top);

    auto *capLay = new QHBoxLayout;
    capLay->addWidget(new QLabel(tr("Capacity utilization")));
    capBar_ = new QProgressBar;
    capBar_->setRange(0, 100);
    capBar_->setValue(0);
    capBar_->setFormat(tr("%p %"));
    capLay->addWidget(capBar_, 1);
    root->addLayout(capLay);

    itemsTable_ = new QTableWidget;
    itemsTable_->setColumnCount(3);
    itemsTable_->setHorizontalHeaderLabels({tr("Item"), tr("Weight"), tr("Relief value")});
    itemsTable_->horizontalHeader()->setStretchLastSection(true);

    // Sample catalog: food, medicine, oxygen, kits, ambulance as high-value bulky item
    struct Row
    {
        const char *name;
        int w;
        int v;
    } rows[] = {{"Food packets (pallet)", 15, 45},   {"Medicine crates", 10, 60},
                {"Oxygen cylinders set", 25, 90},      {"Rescue kits", 8, 35},
                {"Portable hospital module", 40, 95}, {"Water tanks", 18, 40},
                {"Drone survey pack", 6, 30},          {"Ambulance supply bundle", 35, 85}};
    itemsTable_->setRowCount(static_cast<int>(sizeof(rows) / sizeof(rows[0])));
    for (int i = 0; i < itemsTable_->rowCount(); ++i) {
        itemsTable_->setItem(i, 0, new QTableWidgetItem(QString::fromUtf8(rows[i].name)));
        itemsTable_->setItem(i, 1, new QTableWidgetItem(QString::number(rows[i].w)));
        itemsTable_->setItem(i, 2, new QTableWidgetItem(QString::number(rows[i].v)));
    }
    itemsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    itemsTable_->setAlternatingRowColors(true);

    auto *itemsBox = new QGroupBox(tr("Resource items"));
    auto *itemsLay = new QVBoxLayout(itemsBox);
    itemsLay->addWidget(itemsTable_);

    result_ = new QTextEdit;
    result_->setReadOnly(true);
    result_->setPlaceholderText(tr("Optimal loadout and selected items appear here."));

    auto *optBox = new QGroupBox(tr("Optimal allocation"));
    auto *optLay = new QVBoxLayout(optBox);
    optLay->addWidget(result_);

    auto *split = new QHBoxLayout;
    split->addWidget(itemsBox, 1);
    split->addWidget(optBox, 1);
    root->addLayout(split, 1);

    refresh();
}

void ResourcePage::refresh()
{
    // static sample table — nothing to sync from model
}

void ResourcePage::runKnapsack()
{
    std::vector<ReliefItem> items;
    const int n = itemsTable_->rowCount();
    items.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        ReliefItem it;
        it.name = itemsTable_->item(i, 0)->text().toStdString();
        it.weight = itemsTable_->item(i, 1)->text().toInt();
        it.value = itemsTable_->item(i, 2)->text().toInt();
        if (it.weight <= 0)
            it.weight = 1;
        items.push_back(it);
    }
    const int cap = capacity_->value();
    KnapsackResult res = solveReliefKnapsack(items, cap);
    if (capBar_) {
        const int pct = cap > 0 ? std::min(100, (res.usedWeight * 100) / cap) : 0;
        capBar_->setValue(pct);
    }

    QString lines;
    lines += tr("Maximum relief value: %1\nUsed capacity: %2 / %3\n\nSelected items:\n")
                 .arg(res.maxValue)
                 .arg(res.usedWeight)
                 .arg(cap);
    for (int idx : res.selectedIndices) {
        lines += QStringLiteral(" • %1 (w=%2, v=%3)\n")
                     .arg(QString::fromStdString(items[static_cast<std::size_t>(idx)].name))
                     .arg(items[static_cast<std::size_t>(idx)].weight)
                     .arg(items[static_cast<std::size_t>(idx)].value);
    }
    result_->setPlainText(lines);

    model_->history.push({QDateTime::currentDateTime().toString(Qt::ISODate).toStdString(),
                          "Knapsack", "Convoy optimized",
                          "value=" + std::to_string(res.maxValue) + " weight=" + std::to_string(res.usedWeight)});

    QMessageBox::information(this, tr("Knapsack result"),
                             tr("Optimal relief value %1 with capacity usage %2/%3.")
                                 .arg(res.maxValue)
                                 .arg(res.usedWeight)
                                 .arg(cap));
}
