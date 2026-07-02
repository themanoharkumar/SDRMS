#include "settings_page.h"
#include "page_header.h"
#include "page_scroll.h"
#include "thememanager.h"
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent)
{
    auto *root = pageScrollSetup(this);
    addSdrmsPanelHeader(root, 13, tr("System settings"),
                        tr("Configure appearance, notifications, and workspace preferences."));

    // ── Appearance ────────────────────────────────────────────────────────────
    auto *gAppear = new QGroupBox(tr("Appearance"));
    auto *layAppear = new QVBoxLayout(gAppear);

    auto *themeRow = new QHBoxLayout;
    auto *themeLabel = new QLabel(tr("Theme:"));
    auto *btnLight = new QPushButton(QStringLiteral("☀  Light mode"));
    auto *btnDark  = new QPushButton(QStringLiteral("🌙  Dark mode"));
    btnLight->setObjectName(QStringLiteral("btnGhost"));
    btnDark->setObjectName(QStringLiteral("btnGhost"));
    themeRow->addWidget(themeLabel);
    themeRow->addWidget(btnLight);
    themeRow->addWidget(btnDark);
    themeRow->addStretch();
    layAppear->addLayout(themeRow);

    auto *themeNote = new QLabel(tr("Current: ") +
        (ThemeManager::instance()->isDark() ? tr("Dark") : tr("Light")));
    themeNote->setObjectName(QStringLiteral("pageSubtitle"));
    layAppear->addWidget(themeNote);

    // Applying theme also updates top-bar icon via ThemeManager::themeChanged signal
    connect(btnLight, &QPushButton::clicked, this, [themeNote]() {
        ThemeManager::instance()->applyTheme(ThemeManager::Theme::Light);
        themeNote->setText(QObject::tr("Current: Light"));
    });
    connect(btnDark, &QPushButton::clicked, this, [themeNote]() {
        ThemeManager::instance()->applyTheme(ThemeManager::Theme::Dark);
        themeNote->setText(QObject::tr("Current: Dark"));
    });

    // ── Preferences ───────────────────────────────────────────────────────────
    auto *gPref = new QGroupBox(tr("Preferences"));
    auto *layPref = new QVBoxLayout(gPref);
    chkAutoSave_ = new QCheckBox(tr("Prompt before closing if there are unsaved changes"));
    chkNotify_   = new QCheckBox(tr("Show notification badge for new emergency requests"));
    layPref->addWidget(chkAutoSave_);
    layPref->addWidget(chkNotify_);
    layPref->addWidget(new QLabel(tr("Dashboard auto-refresh interval (seconds):")));
    spinRefresh_ = new QSpinBox;
    spinRefresh_->setRange(1, 60);
    spinRefresh_->setValue(2);
    layPref->addWidget(spinRefresh_);
    layPref->addStretch();

    // Load saved preferences
    {
        QSettings s;
        chkAutoSave_->setChecked(s.value(QStringLiteral("prefs/autoSavePrompt"), true).toBool());
        chkNotify_->setChecked(s.value(QStringLiteral("prefs/notifyBadge"), true).toBool());
        spinRefresh_->setValue(s.value(QStringLiteral("prefs/refreshInterval"), 2).toInt());
    }

    auto *btnSave = new QPushButton(tr("Save preferences"));
    connect(btnSave, &QPushButton::clicked, this, &SettingsPage::onSaveClicked);
    layPref->addWidget(btnSave);

    // ── About ─────────────────────────────────────────────────────────────────
    auto *gAbout = new QGroupBox(tr("About SDRMS"));
    auto *layAbout = new QVBoxLayout(gAbout);
    layAbout->addWidget(new QLabel(tr("Smart Disaster Relief Management System")));
    layAbout->addWidget(new QLabel(tr("Version: 2.0   |   Developer: Manohar")));
    layAbout->addWidget(new QLabel(tr("Technology: C++17 + Qt 6.11 + SQLite")));
    layAbout->addWidget(new QLabel(tr("Algorithms: Max-Heap · Knapsack · Dijkstra · TSP · BFS · DFS · Stack")));
    layAbout->addStretch();

    root->addWidget(gAppear);
    root->addWidget(gPref);
    root->addWidget(gAbout);
}

void SettingsPage::onSaveClicked()
{
    // Persist preferences
    QSettings s;
    s.setValue(QStringLiteral("prefs/autoSavePrompt"),  chkAutoSave_->isChecked());
    s.setValue(QStringLiteral("prefs/notifyBadge"),     chkNotify_->isChecked());
    s.setValue(QStringLiteral("prefs/refreshInterval"), spinRefresh_->value());

    // Notify dashboard to apply the new refresh interval
    emit refreshIntervalChanged(spinRefresh_->value());

    QMessageBox::information(this, tr("Settings"),
                             tr("Preferences saved.\nRefresh interval: %1 s")
                                 .arg(spinRefresh_->value()));
}
