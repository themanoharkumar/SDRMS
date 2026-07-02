#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>

class QApplication;

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    enum class Theme { Light, Dark };

    static ThemeManager *instance();

    void applyTheme(Theme t);
    void toggleTheme();
    Theme currentTheme() const { return current_; }
    bool isDark() const { return current_ == Theme::Dark; }

    QString lightSheet() const;
    QString darkSheet()  const;

signals:
    void themeChanged(Theme t);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    static ThemeManager *s_instance;
    Theme current_ = Theme::Light;
};

#endif // THEMEMANAGER_H
