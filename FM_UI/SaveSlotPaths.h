#pragma once

#include"fm/core/GameBootstrapOptions.h"

#include<QString>

namespace SaveSlotPaths {
    QString appDataRoot();
    QString ensureSavesRootDirectory();
    QString savesRootDirectoryPath();
    QString defaultSaveSlotId();
    bool isValidSaveSlotId(const QString& saveSlotId);
    QString requireValidSaveSlotId(const QString& saveSlotId);
    QString saveSlotDirectoryPath(const QString& saveSlotId);
    QString saveDatabasePath(const QString& saveSlotId);
    QString ensureSaveSlotDirectory(const QString& saveSlotId);
    QString ensureSaveDatabasePath(const QString& saveSlotId);

    GameBootstrapOptions createBootstrapOptionsForSaveSlot(
        const QString& saveSlotId,
        DatabaseOpenMode openMode);
}
