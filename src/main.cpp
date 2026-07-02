/**
 * Smart Disaster Relief Management System (SDRMS)
 * Entry point: initialises database, theme, then shows login → dashboard.
 * Supports proper logout: closing the dashboard returns the user to the login screen.
 */
#include <QApplication>
#include <QDir>
#include <QFont>
#include <QMessageBox>
#include <QStyleFactory>
#include "database.h"
#include "thememanager.h"
#include "loginwindow.h"
#include "dashboard.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Smart Disaster Relief Management System"));
    app.setOrganizationName(QStringLiteral("SDRMS"));
    app.setOrganizationDomain(QStringLiteral("sdrms.local"));
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // ── Font ──────────────────────────────────────────────────────────────────
    QFont uiFont(QStringLiteral("Segoe UI"));
    uiFont.setPointSize(10);
    if (!uiFont.exactMatch()) {
        uiFont = QFont();
        uiFont.setPointSize(10);
    }
    app.setFont(uiFont);

    // ── Theme (before any window shown) ───────────────────────────────────────
    ThemeManager *tm = ThemeManager::instance();
    app.setStyleSheet(tm->isDark() ? tm->darkSheet() : tm->lightSheet());

    // ── Database ──────────────────────────────────────────────────────────────
    const QString dataDir =
        QDir::homePath() + QStringLiteral("/SDRMS_Data");
    QDir().mkpath(dataDir);
    const QString dbPath = dataDir + QStringLiteral("/sdrms.db");

    if (!Database::instance()->open(dbPath)) {
        QMessageBox::critical(nullptr, QStringLiteral("Database Error"),
                              QStringLiteral("Failed to open SQLite database at:\n") + dbPath);
        return 1;
    }

    // ── Login / Dashboard loop (supports logout → re-login) ───────────────────
    // We run the login/dashboard cycle in a loop so that when the user logs out
    // (Dashboard emits logoutRequested), we simply hide the dashboard and re-show login.
    Dashboard *dashboard = new Dashboard;
    dashboard->hide();

    bool keepRunning = true;
    while (keepRunning) {
        LoginWindow *login = new LoginWindow;

        bool loggedIn = false;
        QObject::connect(login, &LoginWindow::loginSucceeded, login, [&]() {
            dashboard->setLoggedInUser(login->loggedInUser());
            loggedIn = true;
            login->close();
            dashboard->show();
            dashboard->raise();
            dashboard->activateWindow();
        });

        login->exec(); // blocks until login succeeds or user closes

        if (!loggedIn) {
            // User closed the login window without logging in — exit.
            keepRunning = false;
            break;
        }

        delete login;

        // Wait for the dashboard to be closed (via logout or window X button).
        // Dashboard::logoutRequested signal hides dashboard and we loop back.
        QEventLoop loop;
        QObject::connect(dashboard, &Dashboard::logoutRequested, &loop, [&]() {
            dashboard->hide();
            loop.quit();
        });
        // Also handle the user clicking the window close button (X) — treat as exit.
        QObject::connect(dashboard, &QMainWindow::destroyed, &loop, [&]() {
            keepRunning = false;
            loop.quit();
        });
        loop.exec();

        if (!keepRunning) break;
    }

    Database::instance()->close();
    return 0;
}
