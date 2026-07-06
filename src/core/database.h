#ifndef DATABASE_H
#define DATABASE_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QVector>

// ── User record ──────────────────────────────────────────────────────────────
struct UserRecord {
    int     id        = 0;
    QString username;
    QString fullName;
    QString email;
    QString role;          // "admin" | "user"
    QString createdAt;
    bool    isActive  = true;
};

// ── Notification record ───────────────────────────────────────────────────────
struct NotifRecord {
    int     id        = 0;
    QString title;
    QString message;
    QString type;          // "user_signup" | "disaster" | "system"
    bool    isRead    = false;
    QString createdAt;
};

// ── Message record ────────────────────────────────────────────────────────────
struct MessageRecord {
    int     id        = 0;
    QString sender;
    QString subject;
    QString body;
    bool    isRead    = false;
    QString createdAt;
};

class ApplicationModel;

// ── Database singleton ────────────────────────────────────────────────────────
class Database : public QObject
{
    Q_OBJECT
public:
    static Database *instance();

    bool open(const QString &dbPath);
    void close();
    bool isOpen() const;

    // ── Workspace State Persistence ───────────────────────────────────────────
    bool saveModel(const ApplicationModel &model);
    bool loadModel(ApplicationModel &model);

    // ── Auth ──────────────────────────────────────────────────────────────────
    bool   signUp(const QString &username, const QString &password,
                  const QString &fullName, const QString &email,
                  QString &errorOut);
    bool   login(const QString &username, const QString &password,
                 UserRecord &userOut);
    bool   usernameExists(const QString &username);

    // ── Users ─────────────────────────────────────────────────────────────────
    QVector<UserRecord>   allUsers();
    bool                  setUserActive(int id, bool active);

    // ── Notifications ─────────────────────────────────────────────────────────
    void               pushNotification(const QString &title,
                                        const QString &message,
                                        const QString &type = "system");
    QVector<NotifRecord> notifications(bool unreadOnly = false);
    int                  unreadNotifCount();
    void                 markNotifRead(int id);
    void                 markAllNotifsRead();

    // ── Messages ──────────────────────────────────────────────────────────────
    void               pushMessage(const QString &sender,
                                   const QString &subject,
                                   const QString &body);
    QVector<MessageRecord> messages(bool unreadOnly = false);
    int                    unreadMsgCount();
    void                   markMsgRead(int id);

signals:
    void notificationAdded(const NotifRecord &n);
    void messageAdded(const MessageRecord &m);
    void userAdded(const UserRecord &u);

private:
    explicit Database(QObject *parent = nullptr);
    static Database *s_instance;

    bool createTables();
    QSqlDatabase db_;
};

#endif // DATABASE_H
