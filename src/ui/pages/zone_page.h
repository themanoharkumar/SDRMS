#ifndef ZONE_PAGE_H
#define ZONE_PAGE_H

#include <QWidget>

class ApplicationModel;
class QComboBox;
class QTextEdit;
class QTableWidget;

class ZonePage : public QWidget
{
    Q_OBJECT
public:
    explicit ZonePage(ApplicationModel *model, QWidget *parent = nullptr);
    void refresh();

private slots:
    void runBfs();
    void runDfs();
    void runComponents();

private:
    void reloadStart();

    ApplicationModel *model_;
    QComboBox *start_ = nullptr;
    QTextEdit *output_ = nullptr;
};

#endif // ZONE_PAGE_H
