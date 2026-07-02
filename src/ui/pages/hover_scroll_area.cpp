#include "hover_scroll_area.h"
#include <QApplication>
#include <QCursor>
#include <QEnterEvent>
#include <QScrollBar>

HoverRevealScrollArea::HoverRevealScrollArea(QWidget *parent)
    : QScrollArea(parent)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    hideTimer_.setSingleShot(true);
    hideTimer_.setInterval(200);
    connect(&hideTimer_, &QTimer::timeout, this, [this]() {
        if (!cursorOverSelfOrBars())
            hideScrollBars();
    });

    verticalScrollBar()->installEventFilter(this);
    horizontalScrollBar()->installEventFilter(this);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
}

void HoverRevealScrollArea::enterEvent(QEnterEvent *event)
{
    QScrollArea::enterEvent(event);
    hideTimer_.stop();
    showScrollBars();
}

void HoverRevealScrollArea::leaveEvent(QEvent *event)
{
    QScrollArea::leaveEvent(event);
    scheduleHide();
}

bool HoverRevealScrollArea::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == verticalScrollBar() || watched == horizontalScrollBar()) {
        if (event->type() == QEvent::Enter) {
            hideTimer_.stop();
            showScrollBars();
        } else if (event->type() == QEvent::Leave) {
            scheduleHide();
        }
    }
    return QScrollArea::eventFilter(watched, event);
}

void HoverRevealScrollArea::showScrollBars()
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void HoverRevealScrollArea::hideScrollBars()
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void HoverRevealScrollArea::scheduleHide()
{
    hideTimer_.start();
}

bool HoverRevealScrollArea::cursorOverSelfOrBars() const
{
    const QPoint g = QCursor::pos();
    QWidget *w = QApplication::widgetAt(g);
    for (QWidget *x = w; x != nullptr; x = x->parentWidget()) {
        if (x == this || x == verticalScrollBar() || x == horizontalScrollBar())
            return true;
    }
    return false;
}
