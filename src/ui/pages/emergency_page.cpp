#include "emergency_page.h"
#include "page_header.h"
#include "page_scroll.h"
#include "appdata.h"
#include <QDateTime>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <vector>

EmergencyPage::EmergencyPage(ApplicationModel *model, QWidget *parent)
    : QWidget(parent)
    , model_(model)
{
    auto *root = pageScrollSetup(this);
    addSdrmsPanelHeader(root, 4, tr("Emergency requests (queue)"),
                        tr("FIFO queue — schedule inbound citizen and field requests."));

    auto *form = new QGroupBox(tr("New request"));
    auto *gl = new QGridLayout(form);
    reqType_ = new QComboBox;
    reqType_->addItems({tr("Ambulance"), tr("Food"), tr("Rescue boat"), tr("Shelter"),
                        tr("Medical team"), tr("Water"), tr("Other")});
    loc_   = new QLineEdit;
    notes_ = new QLineEdit;

    int r = 0;
    gl->addWidget(new QLabel(tr("Request type")), r, 0);
    gl->addWidget(reqType_, r, 1);
    ++r;
    gl->addWidget(new QLabel(tr("Location / landmark")), r, 0);
    gl->addWidget(loc_, r, 1);
    ++r;
    gl->addWidget(new QLabel(tr("Notes")), r, 0);
    gl->addWidget(notes_, r, 1);

    auto *row = new QHBoxLayout;
    auto *b1 = new QPushButton(tr("Enqueue request"));
    auto *b2 = new QPushButton(tr("Process next request"));
    b2->setObjectName(QStringLiteral("btnSuccess"));
    connect(b1, &QPushButton::clicked, this, &EmergencyPage::enqueueRequest);
    connect(b2, &QPushButton::clicked, this, &EmergencyPage::dequeueRequest);
    row->addWidget(b1);
    row->addWidget(b2);
    gl->addLayout(row, ++r, 1);

    // Table: ID | Type | Location | Notes | Status | Cancel
    table_ = new QTableWidget;
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels(
        {tr("ID"), tr("Type"), tr("Location"), tr("Notes"), tr("Status"), tr("Cancel")});
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table_->setAlternatingRowColors(true);
    auto *queueBox = new QGroupBox(tr("Request queue (FIFO)"));
    auto *queueLay = new QVBoxLayout(queueBox);
    queueLay->addWidget(table_);

    auto *split = new QHBoxLayout;
    split->addWidget(form, 2);
    split->addWidget(queueBox, 3);
    root->addLayout(split, 1);

    refresh();
}

void EmergencyPage::rebuildTable()
{
    std::vector<EmergencyRequest> snap = model_->emergencyQueue.pendingSnapshot();
    table_->setRowCount(static_cast<int>(snap.size()));
    for (int i = 0; i < static_cast<int>(snap.size()); ++i) {
        const auto &e = snap[static_cast<std::size_t>(i)];
        table_->setItem(i, 0, new QTableWidgetItem(QString::number(e.id)));
        table_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(e.requestType)));
        table_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(e.location)));
        table_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(e.notes)));
        table_->setItem(i, 4, new QTableWidgetItem(tr("Pending")));

        // Cancel button (removes this specific request from the queue)
        auto *btnCancel = new QPushButton(tr("Cancel"));
        btnCancel->setObjectName(QStringLiteral("btnDanger"));
        const long long reqId = e.id;
        const QString reqInfo = QString::fromStdString(e.requestType + " @ " + e.location);
        connect(btnCancel, &QPushButton::clicked, this, [this, reqId, reqInfo]() {
            if (QMessageBox::question(this, tr("Cancel request"),
                                      tr("Cancel request: %1?").arg(reqInfo))
                != QMessageBox::Yes)
                return;
            if (model_->emergencyQueue.cancelById(reqId)) {
                model_->history.push({QDateTime::currentDateTime().toString(Qt::ISODate).toStdString(),
                                      "EmergencyQueue", "Request cancelled",
                                      reqInfo.toStdString()});
                refresh();
            }
        });
        table_->setCellWidget(i, 5, btnCancel);
        table_->setRowHeight(i, 44);
    }
    table_->setColumnWidth(5, 80);
}

void EmergencyPage::refresh()
{
    rebuildTable();
}

void EmergencyPage::enqueueRequest()
{
    if (loc_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Location is required."));
        return;
    }
    EmergencyRequest e;
    e.id = model_->nextEmergencyId();
    e.requestType = reqType_->currentText().toStdString();
    e.location = loc_->text().trimmed().toStdString();
    e.notes = notes_->text().toStdString();
    model_->emergencyQueue.enqueue(e);
    model_->history.push({QDateTime::currentDateTime().toString(Qt::ISODate).toStdString(),
                          "EmergencyQueue", "Request queued",
                          e.requestType + " @ " + e.location});
    refresh();
}

void EmergencyPage::dequeueRequest()
{
    EmergencyRequest e;
    if (!model_->emergencyQueue.dequeue(e)) {
        QMessageBox::information(this, tr("Queue"), tr("No pending requests."));
        return;
    }
    model_->history.push({QDateTime::currentDateTime().toString(Qt::ISODate).toStdString(),
                          "EmergencyQueue", "Request served",
                          e.requestType + " @ " + e.location});
    refresh();
    QMessageBox::information(this, tr("Served"),
                             tr("Processed request %1 — %2")
                                 .arg(e.id)
                                 .arg(QString::fromStdString(e.requestType)));
}
