#ifndef ROUTE_PAGE_H
#define ROUTE_PAGE_H

#include <QWidget>

class ApplicationModel;
class QComboBox;
class QTextEdit;

class RoadGraphWidget;

class RoutePage : public QWidget
{
    Q_OBJECT
public:
    explicit RoutePage(ApplicationModel *model, QWidget *parent = nullptr);
    void refresh();

private slots:
    void runDijkstra();
    void runTsp();
    void toggleBlock();

private:
    void reloadCombos();

    ApplicationModel *model_;
    QComboBox *from_ = nullptr;
    QComboBox *to_ = nullptr;
    QComboBox *blockA_ = nullptr;
    QComboBox *blockB_ = nullptr;
    QTextEdit *output_ = nullptr;
    RoadGraphWidget *graphWidget_ = nullptr;
};

#endif // ROUTE_PAGE_H
