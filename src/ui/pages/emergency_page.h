#ifndef EMERGENCY_PAGE_H
#define EMERGENCY_PAGE_H

#include <QWidget>

class ApplicationModel;
class QTableWidget;
class QComboBox;
class QLineEdit;

class EmergencyPage : public QWidget
{
    Q_OBJECT
public:
    explicit EmergencyPage(ApplicationModel *model, QWidget *parent = nullptr);
    void refresh();

private slots:
    void enqueueRequest();
    void dequeueRequest();

private:
    void rebuildTable();

    ApplicationModel *model_;
    QComboBox *reqType_ = nullptr;
    QLineEdit *loc_ = nullptr;
    QLineEdit *notes_ = nullptr;
    QTableWidget *table_ = nullptr;
};

#endif // EMERGENCY_PAGE_H
