#include "filemanager.h"
#include "appdata.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <algorithm>

namespace {

QString esc(const std::string &s)
{
    return QString::fromStdString(s).replace(QLatin1Char('|'), QLatin1String("\\|"));
}

std::string unesc(QString s)
{
    return s.replace(QLatin1String("\\|"), QLatin1String("|")).toStdString();
}

} // namespace

FileManager::FileManager(QString filePath)
    : path_(std::move(filePath))
{
    if (path_.isEmpty())
        path_ = QDir(QCoreApplication::applicationDirPath())
                    .absoluteFilePath(QStringLiteral("resources/data.txt"));
}

bool FileManager::saveAll(const ApplicationModel &model) const
{
    QFile f(path_);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&f);
    out << "[META]\n";
    out << "version=2\n";
    out << "[DISASTERS]\n";
    for (const auto &d : model.disasters.records()) {
        out << esc(d.disasterType) << '|' << esc(d.location) << '|' << d.severity << '|'
            << d.affectedPeople << '|' << d.teamsNeeded << '|'
            << esc(d.status) << '|' << esc(d.timestamp) << '\n';
    }
    out << "[EMERGENCY]\n";
    for (const auto &e : model.emergencyQueue.pendingSnapshot()) {
        out << e.id << '|' << esc(e.requestType) << '|' << esc(e.location) << '|'
            << esc(e.notes) << '\n';
    }
    out << "[PRIORITY]\n";
    {
        std::vector<PriorityTask> snap = model.priorityHeap.snapshotLevels();
        for (const auto &p : snap) {
            out << p.id << '|' << p.severity << '|' << esc(p.category) << '|'
                << esc(p.description) << '\n';
        }
    }
    out << "[SHELTERS]\n";
    for (const auto &s : model.shelters.all()) {
        out << esc(s.name) << '|' << esc(s.location) << '|' << s.capacity << '|' << s.occupied
            << '\n';
    }
    out << "[HISTORY]\n";
    for (const auto &h : model.history.toVectorOldestFirst()) {
        out << esc(h.timestamp) << '|' << esc(h.operationType) << '|' << esc(h.summary) << '|'
            << esc(h.detail) << '\n';
    }
    out << "[END]\n";
    return true;
}

bool FileManager::loadAll(ApplicationModel &model) const
{
    QFile f(path_);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QTextStream in(&f);

    model.disasters.clear();
    model.emergencyQueue.clear();
    model.priorityHeap.clear();
    model.shelters.clear();
    model.history.clear();
    model.resetIds();

    enum Section { None, Disasters, Emergency, Priority, Shelters, History };
    Section sec = None;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        if (line == QLatin1String("[META]")) {
            sec = None;
            continue;
        }
        if (line == QLatin1String("[DISASTERS]")) {
            sec = Disasters;
            continue;
        }
        if (line == QLatin1String("[EMERGENCY]")) {
            sec = Emergency;
            continue;
        }
        if (line == QLatin1String("[PRIORITY]")) {
            sec = Priority;
            continue;
        }
        if (line == QLatin1String("[SHELTERS]")) {
            sec = Shelters;
            continue;
        }
        if (line == QLatin1String("[HISTORY]")) {
            sec = History;
            continue;
        }
        if (line == QLatin1String("[END]"))
            break;

        if (sec == Disasters) {
            const QStringList p = line.split(QLatin1Char('|'));
            if (p.size() >= 5) {
                DisasterRecord d;
                d.disasterType = unesc(p[0]);
                d.location = unesc(p[1]);
                d.severity = p[2].toInt();
                d.affectedPeople = p[3].toInt();
                d.teamsNeeded = p[4].toInt();
                if (p.size() >= 6) d.status    = unesc(p[5]);
                if (p.size() >= 7) d.timestamp = unesc(p[6]);
                model.disasters.registerDisaster(d);
            }
        } else if (sec == Emergency) {
            const QStringList p = line.split(QLatin1Char('|'));
            if (p.size() >= 4) {
                EmergencyRequest e;
                e.id = p[0].toLongLong();
                e.requestType = unesc(p[1]);
                e.location = unesc(p[2]);
                e.notes = unesc(p[3]);
                model.emergencyQueue.enqueue(e);
            }
        } else if (sec == Priority) {
            const QStringList p = line.split(QLatin1Char('|'));
            if (p.size() >= 4) {
                PriorityTask t;
                t.id = p[0].toLongLong();
                t.severity = p[1].toInt();
                t.category = unesc(p[2]);
                t.description = unesc(p[3]);
                model.priorityHeap.push(t);
            }
        } else if (sec == Shelters) {
            const QStringList p = line.split(QLatin1Char('|'));
            if (p.size() >= 4) {
                ShelterRecord s;
                s.name = unesc(p[0]);
                s.location = unesc(p[1]);
                s.capacity = p[2].toInt();
                s.occupied = p[3].toInt();
                model.shelters.addShelter(s);
            }
        } else if (sec == History) {
            const QStringList p = line.split(QLatin1Char('|'));
            if (p.size() >= 4) {
                HistoryEntry h;
                h.timestamp = unesc(p[0]);
                h.operationType = unesc(p[1]);
                h.summary = unesc(p[2]);
                h.detail = unesc(p[3]);
                model.history.push(h);
            }
        }
    }
    model.reseedCountersFromState();
    return true;
}
