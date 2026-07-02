#ifndef USERS_PAGE_H
#define USERS_PAGE_H

#include <QWidget>
class QTableWidget;
class QLabel;

class UsersPage : public QWidget
{
    Q_OBJECT
public:
    explicit UsersPage(QWidget *parent = nullptr);
    void refresh();

private:
    void toggleUserActive(int id, bool currentlyActive);
    QTableWidget *table_ = nullptr;
    QLabel       *countLabel_ = nullptr;
};

#endif // USERS_PAGE_H
