#include "zone_page.h"
#include "page_header.h"
#include "page_scroll.h"
#include "appdata.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

ZonePage::ZonePage(ApplicationModel *model, QWidget *parent)
    : QWidget(parent)
    , model_(model)
{
    auto *root = pageScrollSetup(this);
    addSdrmsPanelHeader(root, 9, tr("Zone detection (BFS / DFS)"),
                        tr("Graph traversal on the road network to detect reachability and connected components."));

    auto *top = new QGroupBox(tr("Graph traversal (same network as routing)"));
    auto *hl = new QHBoxLayout(top);
    hl->addWidget(new QLabel(tr("Start node")));
    start_ = new QComboBox;
    hl->addWidget(start_, 1);
    auto *b1 = new QPushButton(tr("Run BFS scan"));
    auto *b2 = new QPushButton(tr("Run DFS scan"));
    connect(b1, &QPushButton::clicked, this, &ZonePage::runBfs);
    connect(b2, &QPushButton::clicked, this, &ZonePage::runDfs);
    hl->addWidget(b1);
    hl->addWidget(b2);
    auto *b3 = new QPushButton(tr("List connected components"));
    connect(b3, &QPushButton::clicked, this, &ZonePage::runComponents);
    hl->addWidget(b3);
    root->addWidget(top);

    output_ = new QTextEdit;
    output_->setReadOnly(true);
    root->addWidget(output_, 1);

    refresh();
}

void ZonePage::reloadStart()
{
    start_->clear();
    for (int i = 0; i < model_->roadGraph.nodeCount(); ++i)
        start_->addItem(QString::fromStdString(model_->roadGraph.nodeName(i)), i);
}

void ZonePage::refresh()
{
    reloadStart();
}

void ZonePage::runBfs()
{
    if (start_->count() == 0)
        return;
    int s = start_->currentData().toInt();
    auto ord = model_->roadGraph.bfsOrder(s);
    QString txt = tr("BFS visit order:\n");
    for (int id : ord) {
        txt += QStringLiteral(" • %1\n").arg(QString::fromStdString(model_->roadGraph.nodeName(id)));
    }
    output_->setPlainText(txt);
}

void ZonePage::runDfs()
{
    if (start_->count() == 0)
        return;
    int s = start_->currentData().toInt();
    auto ord = model_->roadGraph.dfsOrder(s);
    QString txt = tr("DFS (preorder) visit order:\n");
    for (int id : ord) {
        txt += QStringLiteral(" • %1\n").arg(QString::fromStdString(model_->roadGraph.nodeName(id)));
    }
    output_->setPlainText(txt);
}

void ZonePage::runComponents()
{
    auto comps = model_->roadGraph.connectedComponents();
    QString txt = tr("Connected components (%1):\n").arg(comps.size());
    for (std::size_t i = 0; i < comps.size(); ++i) {
        txt += tr("Region %1: ").arg(i + 1);
        for (std::size_t j = 0; j < comps[i].size(); ++j) {
            if (j)
                txt += ", ";
            txt += QString::fromStdString(model_->roadGraph.nodeName(comps[i][j]));
        }
        txt += "\n";
    }
    output_->setPlainText(txt);
}
