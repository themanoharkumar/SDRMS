#include "reports_page.h"
#include "page_header.h"
#include "page_scroll.h"
#include "appdata.h"
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QIODevice>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>
#include <sstream>

static std::string buildMissionReport(const ApplicationModel &model)
{
    std::ostringstream o;
    o << "=== DISASTER RELIEF MISSION REPORT ===\n\n";
    o << "Registered disasters: " << model.disasters.records().size() << "\n";
    o << "Pending emergency requests: " << model.emergencyQueue.size() << "\n";
    o << "Priority heap size: " << model.priorityHeap.size() << "\n";
    o << "Shelters tracked: " << model.shelters.all().size() << "\n";
    o << "History entries: " << model.history.size() << "\n\n";

    o << "--- Active disasters ---\n";
    for (const auto &d : model.disasters.records()) {
        o << d.disasterType << " | " << d.location << " | sev " << d.severity << " | affected "
          << d.affectedPeople << " | teams " << d.teamsNeeded << "\n";
    }

    o << "\n--- Pending requests (queue head first) ---\n";
    for (const auto &e : model.emergencyQueue.pendingSnapshot()) {
        o << e.id << " " << e.requestType << " @ " << e.location << " — " << e.notes << "\n";
    }

    o << "\n--- Priority tasks (unsorted heap snapshot) ---\n";
    for (const auto &p : model.priorityHeap.snapshotLevels()) {
        o << p.id << " sev " << p.severity << " " << p.category << " — " << p.description << "\n";
    }

    o << "\n--- Recent operations (oldest first) ---\n";
    for (const auto &h : model.history.toVectorOldestFirst()) {
        o << h.timestamp << " | " << h.operationType << " | " << h.summary << "\n  " << h.detail
          << "\n";
    }

    o << "\n=== END REPORT ===\n";
    return o.str();
}

ReportsPage::ReportsPage(ApplicationModel *model, QWidget *parent)
    : QWidget(parent)
    , model_(model)
{
    auto *root = pageScrollSetup(this);
    addSdrmsPanelHeader(root, 10, tr("Reports & history (stack)"),
                        tr("Mission log on a stack for audit. Undo removes the last recorded operation."));

    summary_ = new QLabel;
    summary_->setObjectName(QStringLiteral("reportsSummaryStrip"));
    summary_->setWordWrap(true);
    root->addWidget(summary_);

    auto *row = new QHBoxLayout;
    auto *b1 = new QPushButton(tr("Refresh"));
    auto *b2 = new QPushButton(tr("Undo last operation"));
    auto *b3 = new QPushButton(tr("Export report to text file"));
    connect(b1, &QPushButton::clicked, this, &ReportsPage::refresh);
    connect(b2, &QPushButton::clicked, this, &ReportsPage::undoLast);
    connect(b3, &QPushButton::clicked, this, &ReportsPage::exportText);
    row->addWidget(b1);
    row->addWidget(b2);
    row->addWidget(b3);
    row->addStretch();
    root->addLayout(row);

    table_ = new QTableWidget;
    table_->setColumnCount(4);
    table_->setHorizontalHeaderLabels({tr("Time"), tr("Type"), tr("Summary"), tr("Detail")});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setAlternatingRowColors(true);
    root->addWidget(table_, 2);

    detail_ = new QTextEdit;
    detail_->setReadOnly(true);
    detail_->setMaximumHeight(140);
    root->addWidget(detail_);

    connect(table_, &QTableWidget::cellClicked, this, [this](int r, int) {
        auto vec = model_->history.toVectorOldestFirst();
        if (r < 0 || r >= static_cast<int>(vec.size()))
            return;
        const auto &h = vec[static_cast<std::size_t>(r)];
        detail_->setPlainText(QString::fromStdString(h.detail));
    });

    refresh();
}

void ReportsPage::rebuildTable()
{
    auto vec = model_->history.toVectorOldestFirst();
    const int n = static_cast<int>(vec.size());
    table_->setRowCount(n);
    for (int i = 0; i < n; ++i) {
        const auto &h = vec[static_cast<std::size_t>(i)];
        table_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(h.timestamp)));
        table_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(h.operationType)));
        table_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(h.summary)));
        table_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(h.detail)));
    }
    if (summary_) {
        summary_->setText(tr("Reports summary — %1 operations on stack; newest at bottom of table. "
                             "Use Undo to pop the last mission log entry.")
                              .arg(n));
    }
}

void ReportsPage::refresh()
{
    rebuildTable();
}

void ReportsPage::undoLast()
{
    HistoryEntry h;
    if (!model_->history.pop(h)) {
        QMessageBox::information(this, tr("History"), tr("Nothing to undo."));
        return;
    }
    QMessageBox::information(this, tr("Undo"),
                             tr("Removed last entry: %1").arg(QString::fromStdString(h.summary)));
    refresh();
}

void ReportsPage::exportText()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save report"), QString(),
                                                 tr("Text files (*.txt)"));
    if (path.isEmpty())
        return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export"), tr("Could not write file."));
        return;
    }
    QTextStream out(&f);
    out << QString::fromStdString(buildMissionReport(*model_));
    out << "\nExported: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    QMessageBox::information(this, tr("Export"), tr("Report saved."));
}
