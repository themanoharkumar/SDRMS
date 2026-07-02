#ifndef PAGE_SCROLL_H
#define PAGE_SCROLL_H

#include "hover_scroll_area.h"
#include <QFrame>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

/**
 * Fills `page` with a vertical QScrollArea and returns the inner QVBoxLayout.
 * Always use stretch factor 1 on the scroll area so the module gets full stack height.
 */
inline QVBoxLayout *pageScrollSetup(QWidget *page, int innerSpacing = 12)
{
    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *scroll = new HoverRevealScrollArea(page);
    scroll->setObjectName(QStringLiteral("moduleScrollArea"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *body = new QWidget;
    body->setObjectName(QStringLiteral("moduleScrollBody"));
    auto *root = new QVBoxLayout(body);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(innerSpacing);

    scroll->setWidget(body);
    outer->addWidget(scroll, 1);
    page->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return root;
}

#endif // PAGE_SCROLL_H
