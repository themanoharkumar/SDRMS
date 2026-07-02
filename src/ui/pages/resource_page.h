#ifndef RESOURCE_PAGE_H
#define RESOURCE_PAGE_H

#include <QWidget>

class ApplicationModel;
class QTableWidget;
class QSpinBox;
class QTextEdit;
class QProgressBar;

class ResourcePage : public QWidget
{
    Q_OBJECT
public:
    explicit ResourcePage(ApplicationModel *model, QWidget *parent = nullptr);
    void refresh();

private slots:
    void runKnapsack();

private:
    ApplicationModel *model_;
    QSpinBox *capacity_ = nullptr;
    QTableWidget *itemsTable_ = nullptr;
    QTextEdit *result_ = nullptr;
    QProgressBar *capBar_ = nullptr;
};

#endif // RESOURCE_PAGE_H
