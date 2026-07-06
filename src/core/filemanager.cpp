#include "filemanager.h"
#include "appdata.h"
#include "database.h"

FileManager::FileManager(QString filePath)
    : path_(std::move(filePath))
{
}

bool FileManager::saveAll(const ApplicationModel &model) const
{
    return Database::instance()->saveModel(model);
}

bool FileManager::loadAll(ApplicationModel &model) const
{
    return Database::instance()->loadModel(model);
}
