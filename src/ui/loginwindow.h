#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>
#include "database.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindow; }
QT_END_NAMESPACE

class QTabWidget;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QLabel;

class LoginWindow : public QDialog
{
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow() override;

    UserRecord loggedInUser() const { return loggedInUser_; }

signals:
    void loginSucceeded();

private slots:
    void tryLogin();
    void trySignUp();

private:
    Ui::LoginWindow *ui;
    UserRecord       loggedInUser_;

    // signup tab widgets
    QLineEdit   *suUser_  = nullptr;
    QLineEdit   *suName_  = nullptr;
    QLineEdit   *suEmail_ = nullptr;
    QLineEdit   *suPass_  = nullptr;
    QLineEdit   *suPass2_ = nullptr;
    QLabel      *suError_ = nullptr;
};

#endif // LOGINWINDOW_H
