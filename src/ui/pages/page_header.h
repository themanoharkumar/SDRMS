#ifndef PAGE_HEADER_H
#define PAGE_HEADER_H

#include <QLabel>
#include <QString>
#include <QHBoxLayout>
#include <QVBoxLayout>

/** Standard page title + subtitle (SDRMS light theme). */
inline void addPageHeader(QVBoxLayout *layout, const QString &title, const QString &subtitle)
{
    auto *lt = new QLabel(title);
    lt->setObjectName(QStringLiteral("pageTitle"));
    auto *ls = new QLabel(subtitle);
    ls->setObjectName(QStringLiteral("pageSubtitle"));
    ls->setWordWrap(true);
    layout->addWidget(lt);
    layout->addWidget(ls);
    layout->addSpacing(14);
}

/** Mockup-style numbered panel header (purple badge). */
inline void addSdrmsPanelHeader(QVBoxLayout *layout, int panelNumber, const QString &panelTitle,
                                const QString &subtitle = QString())
{
    auto *row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(12);
    auto *badge = new QLabel(QString::number(panelNumber));
    badge->setObjectName(QStringLiteral("panelNumberBadge"));
    badge->setFixedSize(36, 36);
    badge->setAlignment(Qt::AlignCenter);
    auto *t = new QLabel(panelTitle);
    t->setObjectName(QStringLiteral("panelHeaderTitle"));
    row->addWidget(badge, 0, Qt::AlignTop);
    row->addWidget(t, 0, Qt::AlignVCenter);
    row->addStretch(1);
    layout->addLayout(row);
    if (!subtitle.isEmpty()) {
        auto *s = new QLabel(subtitle);
        s->setObjectName(QStringLiteral("pageSubtitle"));
        s->setWordWrap(true);
        layout->addWidget(s);
    }
    layout->addSpacing(8);
}

#endif // PAGE_HEADER_H
