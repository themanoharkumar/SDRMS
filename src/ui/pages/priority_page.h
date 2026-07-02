#ifndef PRIORITY_PAGE_H
#define PRIORITY_PAGE_H

#include <QWidget>

class ApplicationModel;
class QTableWidget;
class QComboBox;
class QLineEdit;
class QSpinBox;

class PriorityPage : public QWidget
{
    Q_OBJECT
public:
    explicit PriorityPage(ApplicationModel *model, QWidget *parent = nullptr);
    void refresh();

private slots:
    void addTask();
    void serveHighest();

private:
    void rebuildTable();

    ApplicationModel *model_;
    QComboBox *category_ = nullptr;
    QLineEdit *desc_ = nullptr;
    QSpinBox *severity_ = nullptr;
    QTableWidget *table_ = nullptr;
};

#endif // PRIORITY_PAGE_H
