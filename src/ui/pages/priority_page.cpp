#include "priority_page.h"
#include "page_header.h"
#include "page_scroll.h"
#include "appdata.h"
#include <QDateTime>
#include <QComboBox>
#include <QHBoxLayout>
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
#include <QVBoxLayout>
#include <algorithm>

PriorityPage::PriorityPage(ApplicationModel *model, QWidget *parent)
    : QWidget(parent)
    , model_(model)
{
    auto *root = pageScrollSetup(this);
    addSdrmsPanelHeader(root, 5, tr("Rescue priority (max heap)"),
                        tr("Priority queue by severity — most urgent life-safety work is dispatched first."));

    auto *form = new QGroupBox(tr("Add priority request"));
    auto *gl = new QGridLayout(form);
    category_ = new QComboBox;
    category_->addItems({tr("Medical / ICU"), tr("Child rescue"), tr("Senior evacuation"),
                         tr("Food delivery"), tr("Structural collapse"), tr("Other")});
    desc_ = new QLineEdit;
    severity_ = new QSpinBox;
    severity_->setRange(1, 10);
    severity_->setValue(7);

    int r = 0;
    gl->addWidget(new QLabel(tr("Category")), r, 0);
    gl->addWidget(category_, r, 1);
    ++r;
    gl->addWidget(new QLabel(tr("Description")), r, 0);
    gl->addWidget(desc_, r, 1);
    ++r;
    gl->addWidget(new QLabel(tr("Severity")), r, 0);
    gl->addWidget(severity_, r, 1);

    auto *row = new QHBoxLayout;
    auto *b1 = new QPushButton(tr("Insert into priority queue"));
    auto *b2 = new QPushButton(tr("Serve highest priority"));
    connect(b1, &QPushButton::clicked, this, &PriorityPage::addTask);
    connect(b2, &QPushButton::clicked, this, &PriorityPage::serveHighest);
    row->addWidget(b1);
    row->addWidget(b2);
    gl->addLayout(row, ++r, 1);

    table_ = new QTableWidget;
    table_->setColumnCount(4);
    table_->setHorizontalHeaderLabels({tr("Rank"), tr("Severity"), tr("Category"), tr("Description")});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setAlternatingRowColors(true);
    auto *tableBox = new QGroupBox(tr("Rescue priority queue (max heap)"));
    auto *tblLay = new QVBoxLayout(tableBox);
    tblLay->addWidget(table_);

    auto *split = new QHBoxLayout;
    split->addWidget(form, 2);
    split->addWidget(tableBox, 3);
    root->addLayout(split, 1);

    refresh();
}

void PriorityPage::rebuildTable()
{
    std::vector<PriorityTask> items = model_->priorityHeap.snapshotLevels();
    std::sort(items.begin(), items.end(), [](const PriorityTask &a, const PriorityTask &b) {
        if (a.severity != b.severity)
            return a.severity > b.severity;
        return a.id < b.id;
    });
    table_->setRowCount(static_cast<int>(items.size()));
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        const auto &p = items[static_cast<std::size_t>(i)];
        table_->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        table_->setItem(i, 1, new QTableWidgetItem(QString::number(p.severity)));
        table_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(p.category)));
        auto *descIt = new QTableWidgetItem(QString::fromStdString(p.description));
        descIt->setData(Qt::UserRole, p.id);
        table_->setItem(i, 3, descIt);
    }
}

void PriorityPage::refresh()
{
    rebuildTable();
}

void PriorityPage::addTask()
{
    if (desc_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Please enter a short description."));
        return;
    }
    PriorityTask t;
    t.id = model_->nextPriorityId();
    t.severity = severity_->value();
    t.category = category_->currentText().toStdString();
    t.description = desc_->text().trimmed().toStdString();
    model_->priorityHeap.push(t);
    model_->history.push({QDateTime::currentDateTime().toString(Qt::ISODate).toStdString(),
                          "PriorityHeap", "Task inserted",
                          t.category + " sev=" + std::to_string(t.severity)});
    refresh();
}

void PriorityPage::serveHighest()
{
    PriorityTask t;
    if (!model_->priorityHeap.pop(t)) {
        QMessageBox::information(this, tr("Priority queue"), tr("No tasks pending."));
        return;
    }
    model_->history.push({QDateTime::currentDateTime().toString(Qt::ISODate).toStdString(),
                          "PriorityHeap", "Highest task served",
                          t.category + " — " + t.description});
    refresh();
    QMessageBox::information(this, tr("Dispatched"),
                             tr("Serving task %1 with severity %2")
                                 .arg(t.id)
                                 .arg(t.severity));
}
