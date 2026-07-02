#ifndef SHELTER_PAGE_H
#define SHELTER_PAGE_H

#include <QWidget>

class ApplicationModel;
class QTableWidget;
class QLineEdit;
class QSpinBox;

class ShelterPage : public QWidget
{
    Q_OBJECT
public:
    explicit ShelterPage(ApplicationModel *model, QWidget *parent = nullptr);
    void refresh();

private slots:
    void addShelter();
    void assignBeds();

private:
    void rebuildTable();
    void deleteRow(int row);

    ApplicationModel *model_;
    QLineEdit *name_       = nullptr;
    QLineEdit *loc_        = nullptr;
    QSpinBox  *cap_        = nullptr;
    QSpinBox  *occ_        = nullptr;
    QSpinBox  *assignIdx_  = nullptr;
    QSpinBox  *assignBeds_ = nullptr;
    QTableWidget *table_   = nullptr;
};

#endif // SHELTER_PAGE_H
