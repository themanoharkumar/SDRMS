#include "graph_widget.h"
#include "appdata.h"
#include "thememanager.h"
#include <QPainter>
#include <QPaintEvent>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
}

RoadGraphWidget::RoadGraphWidget(ApplicationModel *model, QWidget *parent)
    : QWidget(parent)
    , model_(model)
{
    setMinimumHeight(320);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void RoadGraphWidget::setHighlightedPath(const std::vector<int> &path)
{
    highlightedPath_ = path;
    update();
}

void RoadGraphWidget::clearHighlight()
{
    highlightedPath_.clear();
    update();
}

void RoadGraphWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    bool isDark = ThemeManager::instance()->isDark();
    QColor bgColor = isDark ? QColor(21, 25, 36) : QColor(245, 247, 250);
    QColor nodeColor = QColor(41, 128, 185);       // Deep blue
    QColor nodeTextColor = isDark ? QColor(240, 240, 240) : QColor(33, 33, 33);
    QColor edgeColor = isDark ? QColor(70, 80, 95) : QColor(190, 195, 200);
    QColor highlightColor = QColor(46, 204, 113);   // Emerald green
    QColor blockedColor = QColor(231, 76, 60);      // Crimson red

    // Fill background
    painter.fillRect(rect(), bgColor);

    int n = model_->roadGraph.nodeCount();
    if (n == 0) return;

    // Calculate node coordinates in a circular layout
    QPoint center(width() / 2, height() / 2);
    int radius = std::min(width(), height()) / 2 - 50;
    if (radius < 40) radius = 40;

    std::vector<QPoint> pts(n);
    for (int i = 0; i < n; ++i) {
        double angle = 2.0 * kPi * i / n - kPi / 2.0;
        pts[i] = QPoint(center.x() + radius * std::cos(angle),
                        center.y() + radius * std::sin(angle));
    }

    // 1. Draw regular & blocked edges
    auto edges = model_->roadGraph.allEdges();
    for (const auto &edge : edges) {
        if (edge.u >= n || edge.v >= n) continue;

        QPen pen;
        if (edge.blocked) {
            pen = QPen(blockedColor, 2, Qt::DashLine);
        } else {
            pen = QPen(edgeColor, 1);
        }
        painter.setPen(pen);
        painter.drawLine(pts[edge.u], pts[edge.v]);

        // Draw weight label in middle of the edge
        QPoint mid = (pts[edge.u] + pts[edge.v]) / 2;
        painter.setPen(isDark ? QColor(160, 170, 185) : QColor(90, 95, 100));
        QString wt = QString::number(edge.weight, 'f', 1) + " km";
        painter.drawText(mid + QPoint(5, -5), wt);

        // Draw blocked 'X' symbol in red
        if (edge.blocked) {
            painter.setPen(QPen(blockedColor, 2));
            int sz = 6;
            painter.drawLine(mid.x() - sz, mid.y() - sz, mid.x() + sz, mid.y() + sz);
            painter.drawLine(mid.x() - sz, mid.y() + sz, mid.x() + sz, mid.y() - sz);
        }
    }

    // 2. Draw highlighted path
    if (highlightedPath_.size() >= 2) {
        painter.setPen(QPen(highlightColor, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        for (std::size_t i = 0; i < highlightedPath_.size() - 1; ++i) {
            int u = highlightedPath_[i];
            int v = highlightedPath_[i+1];
            if (u < n && v < n) {
                painter.drawLine(pts[u], pts[v]);
            }
        }
    }

    // 3. Draw nodes
    int nodeRadius = 18;
    for (int i = 0; i < n; ++i) {
        // Draw node shadow/outline
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 40));
        painter.drawEllipse(pts[i] + QPoint(1, 1), nodeRadius, nodeRadius);

        // Highlight start/end nodes of path
        if (!highlightedPath_.empty()) {
            if (i == highlightedPath_.front()) {
                painter.setPen(QPen(highlightColor, 3));
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(pts[i], nodeRadius + 3, nodeRadius + 3);
            } else if (i == highlightedPath_.back()) {
                painter.setPen(QPen(QColor(241, 196, 15), 3)); // Yellow for destination
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(pts[i], nodeRadius + 3, nodeRadius + 3);
            }
        }

        // Draw node body
        painter.setPen(QPen(isDark ? QColor(41, 128, 185) : QColor(52, 152, 219), 2));
        painter.setBrush(isDark ? QColor(30, 37, 50) : QColor(255, 255, 255));
        painter.drawEllipse(pts[i], nodeRadius, nodeRadius);

        // Draw node index/id inside circle
        painter.setPen(nodeTextColor);
        QFont f = painter.font();
        f.setBold(true);
        f.setPointSize(9);
        painter.setFont(f);
        painter.drawText(QRect(pts[i].x() - nodeRadius, pts[i].y() - nodeRadius, nodeRadius * 2, nodeRadius * 2),
                         Qt::AlignCenter, QString::number(i));

        // Draw label text below node
        QString label = QString::fromStdString(model_->roadGraph.nodeName(i));
        QRect textRect(pts[i].x() - 60, pts[i].y() + nodeRadius + 3, 120, 16);
        painter.drawText(textRect, Qt::AlignCenter, label);
    }
}
