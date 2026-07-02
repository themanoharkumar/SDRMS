#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

#include <QWidget>

class QCheckBox;
class QSpinBox;

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr);

signals:
    /** Emitted when the user saves a new refresh interval (seconds). */
    void refreshIntervalChanged(int seconds);

private slots:
    void onSaveClicked();

private:
    QCheckBox *chkAutoSave_  = nullptr;
    QCheckBox *chkNotify_    = nullptr;
    QSpinBox  *spinRefresh_  = nullptr;
};

#endif // SETTINGS_PAGE_H
