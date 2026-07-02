#ifndef DISASTER_PAGE_H
#define DISASTER_PAGE_H

#include <QWidget>

class ApplicationModel;
class QTableWidget;
class QComboBox;
class QLineEdit;
class QSpinBox;

class DisasterPage : public QWidget
{
    Q_OBJECT
public:
    explicit DisasterPage(ApplicationModel *model, QWidget *parent = nullptr);
    void refresh();

private slots:
    void addDisaster();

private:
    void rebuildTable();
    void deleteRow(int row);
    void cycleStatus(int row);

    ApplicationModel *model_;
    QComboBox *type_     = nullptr;
    QLineEdit *location_ = nullptr;
    QSpinBox  *severity_ = nullptr;
    QSpinBox  *affected_ = nullptr;
    QSpinBox  *teams_    = nullptr;
    QTableWidget *table_ = nullptr;
};

#endif // DISASTER_PAGE_H
