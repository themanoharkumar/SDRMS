#ifndef REPORTS_PAGE_H
#define REPORTS_PAGE_H

#include <QWidget>

class ApplicationModel;
class QTableWidget;
class QTextEdit;
class QLabel;

class ReportsPage : public QWidget
{
    Q_OBJECT
public:
    explicit ReportsPage(ApplicationModel *model, QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void undoLast();
    void exportText();

private:
    void rebuildTable();

    ApplicationModel *model_;
    QTableWidget *table_ = nullptr;
    QTextEdit *detail_ = nullptr;
    QLabel *summary_ = nullptr;
};

#endif // REPORTS_PAGE_H
