#include "loginwindow.h"
#include "ui_loginwindow.h"
#include "database.h"
#include "thememanager.h"
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QVBoxLayout>

LoginWindow::LoginWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    setModal(true);
    setObjectName(QStringLiteral("loginDialog"));
    setWindowTitle(tr("Sign in — SDRMS"));

    // ── Bind QSS Object Names ────────────────────────────────────────────────
    ui->heroFrame->setObjectName(QStringLiteral("loginBrandPanel"));
    ui->heroBrand->setObjectName(QStringLiteral("loginBrandTitle"));
    ui->heroSubtitle->setObjectName(QStringLiteral("loginBrandSub"));
    ui->heroFoot->setObjectName(QStringLiteral("loginTagline"));
    ui->formPanel->setObjectName(QStringLiteral("loginFormCard"));

    // ── Apply current theme ───────────────────────────────────────────────────
    qApp->setStyleSheet(ThemeManager::instance()->isDark()
                            ? ThemeManager::instance()->darkSheet()
                            : ThemeManager::instance()->lightSheet());

    // ── Add Tech Stack Badges to the Left Panel ──────────────────────────────
    auto *badgeRow = new QHBoxLayout;
    badgeRow->setSpacing(6);
    badgeRow->setContentsMargins(0, 8, 0, 0);

    auto makeBadge = [](const QString &text) {
        auto *lbl = new QLabel(text);
        lbl->setObjectName(QStringLiteral("techBadge"));
        lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        return lbl;
    };
    badgeRow->addWidget(makeBadge(QStringLiteral("C++17")));
    badgeRow->addWidget(makeBadge(QStringLiteral("Qt 6.11")));
    badgeRow->addWidget(makeBadge(QStringLiteral("SQLite")));
    badgeRow->addWidget(makeBadge(QStringLiteral("CMake")));
    badgeRow->addStretch();

    // Insert under heroSubtitle (index 2 in heroLayout)
    ui->heroLayout->insertLayout(2, badgeRow);

    // ── Add tab widget into the form panel ────────────────────────────────────
    auto *tabs = new QTabWidget;
    tabs->setObjectName(QStringLiteral("loginTabs"));

    // ── Login tab ─────────────────────────────────────────────────────────────
    auto *loginTab = new QWidget;
    auto *loginLay = new QVBoxLayout(loginTab);
    loginLay->setSpacing(14);
    loginLay->setContentsMargins(0, 8, 0, 0);

    auto *credGroup = new QGroupBox(tr("Credentials"));
    auto *cForm     = new QFormLayout(credGroup);
    cForm->setHorizontalSpacing(14);
    cForm->setVerticalSpacing(12);
    cForm->addRow(tr("Username"), ui->lineUser);
    cForm->addRow(tr("Password"), ui->linePass);

    auto *hintLbl = new QLabel(tr("Default admin: admin / admin123"));
    hintLbl->setObjectName(QStringLiteral("hintLabel"));

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    ui->btnQuit->setObjectName(QStringLiteral("btnGhost"));
    ui->btnLogin->setObjectName(QStringLiteral("btnLoginPrimary"));
    btnRow->addWidget(ui->btnQuit);
    btnRow->addWidget(ui->btnLogin);

    loginLay->addWidget(new QLabel(tr("Sign in to the relief operations console.")));
    loginLay->addWidget(credGroup);
    loginLay->addWidget(ui->chkRemember);
    loginLay->addWidget(hintLbl);
    loginLay->addLayout(btnRow);
    loginLay->addStretch();

    // ── Sign-up tab ───────────────────────────────────────────────────────────
    auto *signupTab = new QWidget;
    auto *signupLay = new QVBoxLayout(signupTab);
    signupLay->setSpacing(14);
    signupLay->setContentsMargins(0, 8, 0, 0);

    auto *suGroup = new QGroupBox(tr("Create new account"));
    auto *sForm   = new QFormLayout(suGroup);
    sForm->setHorizontalSpacing(14);
    sForm->setVerticalSpacing(12);

    suUser_  = new QLineEdit; suUser_->setPlaceholderText(tr("e.g. john_doe"));
    suName_  = new QLineEdit; suName_->setPlaceholderText(tr("Full name"));
    suEmail_ = new QLineEdit; suEmail_->setPlaceholderText(tr("email@example.com"));
    suPass_  = new QLineEdit; suPass_->setEchoMode(QLineEdit::Password);
    suPass_->setPlaceholderText(tr("At least 6 characters"));
    suPass2_ = new QLineEdit; suPass2_->setEchoMode(QLineEdit::Password);
    suPass2_->setPlaceholderText(tr("Repeat password"));

    sForm->addRow(tr("Username *"), suUser_);
    sForm->addRow(tr("Full name"),  suName_);
    sForm->addRow(tr("Email"),      suEmail_);
    sForm->addRow(tr("Password *"), suPass_);
    sForm->addRow(tr("Confirm *"),  suPass2_);

    suError_ = new QLabel;
    suError_->setObjectName(QStringLiteral("suErrorLabel"));
    suError_->setWordWrap(true);
    suError_->hide();

    auto *btnSignUp = new QPushButton(tr("Create account"));
    btnSignUp->setObjectName(QStringLiteral("btnLoginPrimary"));
    connect(btnSignUp, &QPushButton::clicked, this, &LoginWindow::trySignUp);
    connect(suPass2_, &QLineEdit::returnPressed, this, &LoginWindow::trySignUp);

    signupLay->addWidget(new QLabel(tr("Register a new user account.")));
    signupLay->addWidget(suGroup);
    signupLay->addWidget(suError_);
    signupLay->addWidget(btnSignUp);
    signupLay->addStretch();

    tabs->addTab(loginTab,  tr("Login"));
    tabs->addTab(signupTab, tr("Sign up"));

    // ── Replace form panel content ────────────────────────────────────────────
    auto *fp = ui->formVertical;
    // Remove existing items (we manage them inside tabs now)
    while (QLayoutItem *item = fp->takeAt(0)) {
        if (item->widget()) item->widget()->setParent(nullptr);
        delete item;
    }
    auto *titleLbl = new QLabel(tr("Welcome back"));
    titleLbl->setObjectName(QStringLiteral("loginFormTitle"));
    auto *subLbl   = new QLabel(tr("SDRMS operations terminal"));
    subLbl->setObjectName(QStringLiteral("loginFormSub"));

    fp->addWidget(titleLbl);
    fp->addWidget(subLbl);
    fp->addWidget(tabs);

    // ── Saved username ────────────────────────────────────────────────────────
    QSettings settings;
    if (settings.value(QStringLiteral("login/remember"), false).toBool()) {
        ui->chkRemember->setChecked(true);
        ui->lineUser->setText(settings.value(QStringLiteral("login/user")).toString());
    }

    connect(ui->btnLogin, &QPushButton::clicked, this, &LoginWindow::tryLogin);
    connect(ui->btnQuit,  &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->linePass, &QLineEdit::returnPressed, this, &LoginWindow::tryLogin);
}

LoginWindow::~LoginWindow() { delete ui; }

void LoginWindow::tryLogin()
{
    const QString u = ui->lineUser->text().trimmed();
    const QString p = ui->linePass->text();

    UserRecord user;
    if (!Database::instance()->login(u, p, user)) {
        QMessageBox::warning(this, tr("Login failed"),
                             tr("Invalid username or password.\n"
                                "Check your credentials or create a new account."));
        ui->linePass->clear();
        ui->linePass->setFocus();
        return;
    }

    QSettings settings;
    if (ui->chkRemember->isChecked()) {
        settings.setValue(QStringLiteral("login/remember"), true);
        settings.setValue(QStringLiteral("login/user"), u);
    } else {
        settings.setValue(QStringLiteral("login/remember"), false);
        settings.remove(QStringLiteral("login/user"));
    }

    loggedInUser_ = user;
    emit loginSucceeded();
    accept();
}

void LoginWindow::trySignUp()
{
    if (!suError_ || !suUser_ || !suPass_ || !suPass2_) return;

    const QString u  = suUser_->text().trimmed();
    const QString fn = suName_->text().trimmed();
    const QString em = suEmail_->text().trimmed();
    const QString p1 = suPass_->text();
    const QString p2 = suPass2_->text();

    suError_->hide();

    if (p1 != p2) {
        suError_->setText(tr("Passwords do not match."));
        suError_->show();
        return;
    }

    QString err;
    if (!Database::instance()->signUp(u, p1, fn, em, err)) {
        suError_->setText(err);
        suError_->show();
        return;
    }

    suUser_->clear(); suName_->clear(); suEmail_->clear();
    suPass_->clear(); suPass2_->clear();
    QMessageBox::information(this, tr("Account created"),
                             tr("Your account has been created!\n"
                                "You can now log in with your credentials."));
}
