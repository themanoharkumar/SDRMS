#ifndef HOVER_SCROLL_AREA_H
#define HOVER_SCROLL_AREA_H

#include <QScrollArea>
#include <QTimer>

class HoverRevealScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    explicit HoverRevealScrollArea(QWidget *parent = nullptr);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void showScrollBars();
    void hideScrollBars();
    void scheduleHide();
    bool cursorOverSelfOrBars() const;

    QTimer hideTimer_;
};

#endif // HOVER_SCROLL_AREA_H
