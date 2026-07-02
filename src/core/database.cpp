#include "database.h"

#include <QCryptographicHash>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

Database *Database::s_instance = nullptr;

Database *Database::instance()
{
    if (!s_instance)
        s_instance = new Database;
    return s_instance;
}

Database::Database(QObject *parent) : QObject(parent) {}

// ── open / close ──────────────────────────────────────────────────────────────
bool Database::open(const QString &dbPath)
{
    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("sdrms_db"));
    db_.setDatabaseName(dbPath);
    if (!db_.open()) {
        qWarning() << "DB open error:" << db_.lastError().text();
        return false;
    }
    QSqlQuery q(db_);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL;"));
    q.exec(QStringLiteral("PRAGMA foreign_keys=ON;"));
    return createTables();
}

void Database::close()
{
    db_.close();
}

bool Database::isOpen() const { return db_.isOpen(); }

// ── private helpers ───────────────────────────────────────────────────────────
static QString hashPwd(const QString &pwd)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(pwd.toUtf8(), QCryptographicHash::Sha256).toHex());
}

bool Database::createTables()
{
    QSqlQuery q(db_);

    // users
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS users ("
            "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  username    TEXT    UNIQUE NOT NULL,"
            "  password    TEXT    NOT NULL,"
            "  full_name   TEXT    DEFAULT '',"
            "  email       TEXT    DEFAULT '',"
            "  role        TEXT    DEFAULT 'user',"
            "  is_active   INTEGER DEFAULT 1,"
            "  created_at  TEXT    DEFAULT (datetime('now','localtime'))"
            ");"))) {
        qWarning() << "users table:" << q.lastError().text();
        return false;
    }

    // Seed default admin if not present
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM users WHERE username='admin';"));
    q.exec();
    if (q.next() && q.value(0).toInt() == 0) {
        q.prepare(QStringLiteral(
            "INSERT INTO users (username,password,full_name,email,role)"
            " VALUES ('admin',:pw,'Administrator','admin@sdrms.local','admin');"));
        q.bindValue(QStringLiteral(":pw"), hashPwd(QStringLiteral("admin123")));
        q.exec();
    }

    // notifications
    q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS notifications ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  title      TEXT    NOT NULL,"
        "  message    TEXT    DEFAULT '',"
        "  type       TEXT    DEFAULT 'system',"
        "  is_read    INTEGER DEFAULT 0,"
        "  created_at TEXT    DEFAULT (datetime('now','localtime'))"
        ");"));

    // messages
    q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sender     TEXT    NOT NULL,"
        "  subject    TEXT    DEFAULT '',"
        "  body       TEXT    DEFAULT '',"
        "  is_read    INTEGER DEFAULT 0,"
        "  created_at TEXT    DEFAULT (datetime('now','localtime'))"
        ");"));

    return true;
}

// ── Auth ──────────────────────────────────────────────────────────────────────
bool Database::usernameExists(const QString &username)
{
    QSqlQuery q(db_);
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM users WHERE username=:u;"));
    q.bindValue(QStringLiteral(":u"), username.toLower().trimmed());
    q.exec();
    return q.next() && q.value(0).toInt() > 0;
}

bool Database::signUp(const QString &username, const QString &password,
                      const QString &fullName, const QString &email,
                      QString &errorOut)
{
    const QString u = username.toLower().trimmed();
    if (u.isEmpty()) { errorOut = "Username cannot be empty."; return false; }
    if (password.length() < 6) { errorOut = "Password must be at least 6 characters."; return false; }
    if (usernameExists(u)) { errorOut = "Username already taken."; return false; }

    QSqlQuery q(db_);
    q.prepare(QStringLiteral(
        "INSERT INTO users (username,password,full_name,email,role)"
        " VALUES (:u,:pw,:fn,:em,'user');"));
    q.bindValue(QStringLiteral(":u"),  u);
    q.bindValue(QStringLiteral(":pw"), hashPwd(password));
    q.bindValue(QStringLiteral(":fn"), fullName.trimmed());
    q.bindValue(QStringLiteral(":em"), email.trimmed());
    if (!q.exec()) { errorOut = q.lastError().text(); return false; }

    int newId = q.lastInsertId().toInt();
    UserRecord ur;
    ur.id = newId; ur.username = u; ur.fullName = fullName.trimmed();
    ur.email = email.trimmed(); ur.role = "user"; ur.isActive = true;
    ur.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);

    // push admin notification
    pushNotification(
        QStringLiteral("New user registered"),
        QStringLiteral("User '%1' (%2) signed up.").arg(u, fullName.trimmed()),
        QStringLiteral("user_signup"));

    emit userAdded(ur);
    return true;
}

bool Database::login(const QString &username, const QString &password, UserRecord &userOut)
{
    QSqlQuery q(db_);
    q.prepare(QStringLiteral(
        "SELECT id,username,full_name,email,role,is_active,created_at"
        " FROM users WHERE username=:u AND password=:pw AND is_active=1;"));
    q.bindValue(QStringLiteral(":u"),  username.toLower().trimmed());
    q.bindValue(QStringLiteral(":pw"), hashPwd(password));
    if (!q.exec() || !q.next()) return false;

    userOut.id        = q.value(0).toInt();
    userOut.username  = q.value(1).toString();
    userOut.fullName  = q.value(2).toString();
    userOut.email     = q.value(3).toString();
    userOut.role      = q.value(4).toString();
    userOut.isActive  = q.value(5).toBool();
    userOut.createdAt = q.value(6).toString();
    return true;
}

// ── Users ─────────────────────────────────────────────────────────────────────
QVector<UserRecord> Database::allUsers()
{
    QVector<UserRecord> out;
    QSqlQuery q(db_);
    q.exec(QStringLiteral(
        "SELECT id,username,full_name,email,role,is_active,created_at"
        " FROM users ORDER BY id ASC;"));
    while (q.next()) {
        UserRecord r;
        r.id        = q.value(0).toInt();
        r.username  = q.value(1).toString();
        r.fullName  = q.value(2).toString();
        r.email     = q.value(3).toString();
        r.role      = q.value(4).toString();
        r.isActive  = q.value(5).toBool();
        r.createdAt = q.value(6).toString();
        out.append(r);
    }
    return out;
}

bool Database::setUserActive(int id, bool active)
{
    QSqlQuery q(db_);
    q.prepare(QStringLiteral("UPDATE users SET is_active=:a WHERE id=:id;"));
    q.bindValue(QStringLiteral(":a"),  active ? 1 : 0);
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

// ── Notifications ─────────────────────────────────────────────────────────────
void Database::pushNotification(const QString &title, const QString &message, const QString &type)
{
    QSqlQuery q(db_);
    q.prepare(QStringLiteral(
        "INSERT INTO notifications (title,message,type) VALUES (:t,:m,:tp);"));
    q.bindValue(QStringLiteral(":t"),  title);
    q.bindValue(QStringLiteral(":m"),  message);
    q.bindValue(QStringLiteral(":tp"), type);
    q.exec();

    NotifRecord n;
    n.id        = q.lastInsertId().toInt();
    n.title     = title;
    n.message   = message;
    n.type      = type;
    n.isRead    = false;
    n.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    emit notificationAdded(n);
}

QVector<NotifRecord> Database::notifications(bool unreadOnly)
{
    QVector<NotifRecord> out;
    QSqlQuery q(db_);
    if (unreadOnly)
        q.exec(QStringLiteral(
            "SELECT id,title,message,type,is_read,created_at"
            " FROM notifications WHERE is_read=0 ORDER BY id DESC LIMIT 50;"));
    else
        q.exec(QStringLiteral(
            "SELECT id,title,message,type,is_read,created_at"
            " FROM notifications ORDER BY id DESC LIMIT 50;"));
    while (q.next()) {
        NotifRecord n;
        n.id        = q.value(0).toInt();
        n.title     = q.value(1).toString();
        n.message   = q.value(2).toString();
        n.type      = q.value(3).toString();
        n.isRead    = q.value(4).toBool();
        n.createdAt = q.value(5).toString();
        out.append(n);
    }
    return out;
}

int Database::unreadNotifCount()
{
    QSqlQuery q(db_);
    q.exec(QStringLiteral("SELECT COUNT(*) FROM notifications WHERE is_read=0;"));
    return q.next() ? q.value(0).toInt() : 0;
}

void Database::markNotifRead(int id)
{
    QSqlQuery q(db_);
    q.prepare(QStringLiteral("UPDATE notifications SET is_read=1 WHERE id=:id;"));
    q.bindValue(QStringLiteral(":id"), id);
    q.exec();
}

void Database::markAllNotifsRead()
{
    QSqlQuery q(db_);
    q.exec(QStringLiteral("UPDATE notifications SET is_read=1;"));
}

// ── Messages ──────────────────────────────────────────────────────────────────
void Database::pushMessage(const QString &sender, const QString &subject, const QString &body)
{
    QSqlQuery q(db_);
    q.prepare(QStringLiteral(
        "INSERT INTO messages (sender,subject,body) VALUES (:s,:sub,:b);"));
    q.bindValue(QStringLiteral(":s"),   sender);
    q.bindValue(QStringLiteral(":sub"), subject);
    q.bindValue(QStringLiteral(":b"),   body);
    q.exec();

    MessageRecord m;
    m.id        = q.lastInsertId().toInt();
    m.sender    = sender;
    m.subject   = subject;
    m.body      = body;
    m.isRead    = false;
    m.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    emit messageAdded(m);
}

QVector<MessageRecord> Database::messages(bool unreadOnly)
{
    QVector<MessageRecord> out;
    QSqlQuery q(db_);
    if (unreadOnly)
        q.exec(QStringLiteral(
            "SELECT id,sender,subject,body,is_read,created_at"
            " FROM messages WHERE is_read=0 ORDER BY id DESC LIMIT 50;"));
    else
        q.exec(QStringLiteral(
            "SELECT id,sender,subject,body,is_read,created_at"
            " FROM messages ORDER BY id DESC LIMIT 50;"));
    while (q.next()) {
        MessageRecord m;
        m.id        = q.value(0).toInt();
        m.sender    = q.value(1).toString();
        m.subject   = q.value(2).toString();
        m.body      = q.value(3).toString();
        m.isRead    = q.value(4).toBool();
        m.createdAt = q.value(5).toString();
        out.append(m);
    }
    return out;
}

int Database::unreadMsgCount()
{
    QSqlQuery q(db_);
    q.exec(QStringLiteral("SELECT COUNT(*) FROM messages WHERE is_read=0;"));
    return q.next() ? q.value(0).toInt() : 0;
}

void Database::markMsgRead(int id)
{
    QSqlQuery q(db_);
    q.prepare(QStringLiteral("UPDATE messages SET is_read=1 WHERE id=:id;"));
    q.bindValue(QStringLiteral(":id"), id);
    q.exec();
}
