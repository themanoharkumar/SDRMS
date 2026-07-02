#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QString>

class ApplicationModel;

/**
 * Persists application state to a text file (portable, no DB dependency).
 */
class FileManager
{
public:
    explicit FileManager(QString filePath = {});

    bool saveAll(const ApplicationModel &model) const;
    bool loadAll(ApplicationModel &model) const;

    QString filePath() const { return path_; }
    void setFilePath(const QString &p) { path_ = p; }

private:
    QString path_;
};

#endif // FILEMANAGER_H
