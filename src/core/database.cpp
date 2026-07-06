#include "database.h"
#include "appdata.h"

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
    if (!q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS notifications ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  title      TEXT    NOT NULL,"
        "  message    TEXT    DEFAULT '',"
        "  type       TEXT    DEFAULT 'system',"
        "  is_read    INTEGER DEFAULT 0,"
        "  created_at TEXT    DEFAULT (datetime('now','localtime'))"
        ");"))) {
        qWarning() << "notifications table error:" << q.lastError().text();
        return false;
    }

    // messages
    if (!q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sender     TEXT    NOT NULL,"
        "  subject    TEXT    DEFAULT '',"
        "  body       TEXT    DEFAULT '',"
        "  is_read    INTEGER DEFAULT 0,"
        "  created_at TEXT    DEFAULT (datetime('now','localtime'))"
        ");"))) {
        qWarning() << "messages table error:" << q.lastError().text();
        return false;
    }

    // disasters
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS disasters ("
            "  id              INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  disaster_type   TEXT    NOT NULL,"
            "  location        TEXT    NOT NULL,"
            "  severity        INTEGER NOT NULL,"
            "  affected_people INTEGER NOT NULL,"
            "  teams_needed    INTEGER NOT NULL,"
            "  status          TEXT    NOT NULL,"
            "  timestamp       TEXT    NOT NULL"
            ");"))) {
        qWarning() << "disasters table error:" << q.lastError().text();
        return false;
    }

    // emergency_requests
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS emergency_requests ("
            "  id             INTEGER PRIMARY KEY,"
            "  request_type   TEXT    NOT NULL,"
            "  location       TEXT    NOT NULL,"
            "  notes          TEXT"
            ");"))) {
        qWarning() << "emergency_requests table error:" << q.lastError().text();
        return false;
    }

    // priority_tasks
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS priority_tasks ("
            "  id          INTEGER PRIMARY KEY,"
            "  severity    INTEGER NOT NULL,"
            "  category    TEXT    NOT NULL,"
            "  description TEXT"
            ");"))) {
        qWarning() << "priority_tasks table error:" << q.lastError().text();
        return false;
    }

    // shelters
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS shelters ("
            "  id       INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  name     TEXT    UNIQUE NOT NULL,"
            "  location TEXT    NOT NULL,"
            "  capacity INTEGER NOT NULL,"
            "  occupied INTEGER NOT NULL"
            ");"))) {
        qWarning() << "shelters table error:" << q.lastError().text();
        return false;
    }

    // operation_history
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS operation_history ("
            "  id             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  timestamp      TEXT    NOT NULL,"
            "  operation_type TEXT    NOT NULL,"
            "  summary        TEXT    NOT NULL,"
            "  detail         TEXT"
            ");"))) {
        qWarning() << "operation_history table error:" << q.lastError().text();
        return false;
    }

    // graph_nodes
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS graph_nodes ("
            "  id    INTEGER PRIMARY KEY,"
            "  name  TEXT    UNIQUE NOT NULL"
            ");"))) {
        qWarning() << "graph_nodes table error:" << q.lastError().text();
        return false;
    }

    // graph_edges
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS graph_edges ("
            "  u        INTEGER NOT NULL,"
            "  v        INTEGER NOT NULL,"
            "  weight   REAL    NOT NULL,"
            "  blocked  INTEGER DEFAULT 0,"
            "  PRIMARY KEY(u, v)"
            ");"))) {
        qWarning() << "graph_edges table error:" << q.lastError().text();
        return false;
    }

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

// ── Workspace State Persistence ───────────────────────────────────────────
bool Database::saveModel(const ApplicationModel &model)
{
    if (!isOpen())
        return false;

    db_.transaction();

    QSqlQuery q(db_);
    
    // Clear old data
    q.exec(QStringLiteral("DELETE FROM disasters;"));
    q.exec(QStringLiteral("DELETE FROM emergency_requests;"));
    q.exec(QStringLiteral("DELETE FROM priority_tasks;"));
    q.exec(QStringLiteral("DELETE FROM shelters;"));
    q.exec(QStringLiteral("DELETE FROM operation_history;"));

    // Save disasters
    q.prepare(QStringLiteral(
        "INSERT INTO disasters (disaster_type, location, severity, affected_people, teams_needed, status, timestamp) "
        "VALUES (:type, :loc, :sev, :aff, :teams, :stat, :time);"));
    for (const auto &d : model.disasters.records()) {
        q.bindValue(QStringLiteral(":type"), QString::fromStdString(d.disasterType));
        q.bindValue(QStringLiteral(":loc"), QString::fromStdString(d.location));
        q.bindValue(QStringLiteral(":sev"), d.severity);
        q.bindValue(QStringLiteral(":aff"), d.affectedPeople);
        q.bindValue(QStringLiteral(":teams"), d.teamsNeeded);
        q.bindValue(QStringLiteral(":stat"), QString::fromStdString(d.status));
        q.bindValue(QStringLiteral(":time"), QString::fromStdString(d.timestamp));
        q.exec();
    }

    // Save emergency requests
    q.prepare(QStringLiteral(
        "INSERT INTO emergency_requests (id, request_type, location, notes) "
        "VALUES (:id, :type, :loc, :notes);"));
    for (const auto &e : model.emergencyQueue.pendingSnapshot()) {
        q.bindValue(QStringLiteral(":id"), static_cast<qlonglong>(e.id));
        q.bindValue(QStringLiteral(":type"), QString::fromStdString(e.requestType));
        q.bindValue(QStringLiteral(":loc"), QString::fromStdString(e.location));
        q.bindValue(QStringLiteral(":notes"), QString::fromStdString(e.notes));
        q.exec();
    }

    // Save priority tasks
    q.prepare(QStringLiteral(
        "INSERT INTO priority_tasks (id, severity, category, description) "
        "VALUES (:id, :sev, :cat, :desc);"));
    for (const auto &p : model.priorityHeap.snapshotLevels()) {
        q.bindValue(QStringLiteral(":id"), static_cast<qlonglong>(p.id));
        q.bindValue(QStringLiteral(":sev"), p.severity);
        q.bindValue(QStringLiteral(":cat"), QString::fromStdString(p.category));
        q.bindValue(QStringLiteral(":desc"), QString::fromStdString(p.description));
        q.exec();
    }

    // Save shelters
    q.prepare(QStringLiteral(
        "INSERT INTO shelters (name, location, capacity, occupied) "
        "VALUES (:name, :loc, :cap, :occ);"));
    for (const auto &s : model.shelters.all()) {
        q.bindValue(QStringLiteral(":name"), QString::fromStdString(s.name));
        q.bindValue(QStringLiteral(":loc"), QString::fromStdString(s.location));
        q.bindValue(QStringLiteral(":cap"), s.capacity);
        q.bindValue(QStringLiteral(":occ"), s.occupied);
        q.exec();
    }

    // Save history
    q.prepare(QStringLiteral(
        "INSERT INTO operation_history (timestamp, operation_type, summary, detail) "
        "VALUES (:time, :type, :sum, :det);"));
    for (const auto &h : model.history.toVectorOldestFirst()) {
        q.bindValue(QStringLiteral(":time"), QString::fromStdString(h.timestamp));
        q.bindValue(QStringLiteral(":type"), QString::fromStdString(h.operationType));
        q.bindValue(QStringLiteral(":sum"), QString::fromStdString(h.summary));
        q.bindValue(QStringLiteral(":det"), QString::fromStdString(h.detail));
        q.exec();
    }

    // Save graph nodes
    q.exec(QStringLiteral("DELETE FROM graph_nodes;"));
    q.prepare(QStringLiteral("INSERT INTO graph_nodes (id, name) VALUES (:id, :name);"));
    for (int i = 0; i < model.roadGraph.nodeCount(); ++i) {
        q.bindValue(QStringLiteral(":id"), i);
        q.bindValue(QStringLiteral(":name"), QString::fromStdString(model.roadGraph.nodeName(i)));
        q.exec();
    }

    // Save graph edges
    q.exec(QStringLiteral("DELETE FROM graph_edges;"));
    q.prepare(QStringLiteral("INSERT OR IGNORE INTO graph_edges (u, v, weight, blocked) VALUES (:u, :v, :w, :b);"));
    for (const auto &edge : model.roadGraph.allEdges()) {
        q.bindValue(QStringLiteral(":u"), edge.u);
        q.bindValue(QStringLiteral(":v"), edge.v);
        q.bindValue(QStringLiteral(":w"), edge.weight);
        q.bindValue(QStringLiteral(":b"), edge.blocked ? 1 : 0);
        q.exec();
    }

    return db_.commit();
}

bool Database::loadModel(ApplicationModel &model)
{
    if (!isOpen())
        return false;

    QSqlQuery q(db_);

    // Check if tables are empty
    q.exec(QStringLiteral("SELECT COUNT(*) FROM disasters;"));
    int dCount = q.next() ? q.value(0).toInt() : 0;
    q.exec(QStringLiteral("SELECT COUNT(*) FROM shelters;"));
    int sCount = q.next() ? q.value(0).toInt() : 0;

    if (dCount == 0 && sCount == 0) {
        // Fresh database. Keep constructor-populated sample data.
        return true;
    }

    model.disasters.clear();
    model.emergencyQueue.clear();
    model.priorityHeap.clear();
    model.shelters.clear();
    model.history.clear();
    model.resetIds();

    // Load disasters
    if (q.exec(QStringLiteral("SELECT disaster_type, location, severity, affected_people, teams_needed, status, timestamp FROM disasters;"))) {
        while (q.next()) {
            DisasterRecord d;
            d.disasterType = q.value(0).toString().toStdString();
            d.location = q.value(1).toString().toStdString();
            d.severity = q.value(2).toInt();
            d.affectedPeople = q.value(3).toInt();
            d.teamsNeeded = q.value(4).toInt();
            d.status = q.value(5).toString().toStdString();
            d.timestamp = q.value(6).toString().toStdString();
            model.disasters.registerDisaster(d);
        }
    }

    // Load emergency requests
    if (q.exec(QStringLiteral("SELECT id, request_type, location, notes FROM emergency_requests;"))) {
        while (q.next()) {
            EmergencyRequest e;
            e.id = q.value(0).toLongLong();
            e.requestType = q.value(1).toString().toStdString();
            e.location = q.value(2).toString().toStdString();
            e.notes = q.value(3).toString().toStdString();
            model.emergencyQueue.enqueue(e);
        }
    }

    // Load priority tasks
    if (q.exec(QStringLiteral("SELECT id, severity, category, description FROM priority_tasks;"))) {
        while (q.next()) {
            PriorityTask p;
            p.id = q.value(0).toLongLong();
            p.severity = q.value(1).toInt();
            p.category = q.value(2).toString().toStdString();
            p.description = q.value(3).toString().toStdString();
            model.priorityHeap.push(p);
        }
    }

    // Load shelters
    if (q.exec(QStringLiteral("SELECT name, location, capacity, occupied FROM shelters;"))) {
        while (q.next()) {
            ShelterRecord s;
            s.name = q.value(0).toString().toStdString();
            s.location = q.value(1).toString().toStdString();
            s.capacity = q.value(2).toInt();
            s.occupied = q.value(3).toInt();
            model.shelters.addShelter(s);
        }
    }

    // Load history
    if (q.exec(QStringLiteral("SELECT timestamp, operation_type, summary, detail FROM operation_history ORDER BY id ASC;"))) {
        while (q.next()) {
            HistoryEntry h;
            h.timestamp = q.value(0).toString().toStdString();
            h.operationType = q.value(1).toString().toStdString();
            h.summary = q.value(2).toString().toStdString();
            h.detail = q.value(3).toString().toStdString();
            model.history.push(h);
        }
    }

    // Load graph nodes and edges
    q.exec(QStringLiteral("SELECT COUNT(*) FROM graph_nodes;"));
    int nodeCount = q.next() ? q.value(0).toInt() : 0;
    if (nodeCount > 0) {
        model.roadGraph.clear();

        // Load graph nodes
        if (q.exec(QStringLiteral("SELECT id, name FROM graph_nodes ORDER BY id ASC;"))) {
            while (q.next()) {
                std::string name = q.value(1).toString().toStdString();
                model.roadGraph.addNode(name);
            }
        }

        // Load graph edges
        if (q.exec(QStringLiteral("SELECT u, v, weight, blocked FROM graph_edges;"))) {
            while (q.next()) {
                int u = q.value(0).toInt();
                int v = q.value(1).toInt();
                double w = q.value(2).toDouble();
                bool b = q.value(3).toBool();
                model.roadGraph.addEdge(u, v, w, b);
            }
        }
    }

    // Reseed generator counters
    model.reseedCountersFromState();
    return true;
}
