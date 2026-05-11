#pragma once

#include"fm/core/GameBootstrapOptions.h"

#include<QString>

namespace SaveSlotPaths {
    QString appDataRoot();
    QString savesRootDirectory();
    QString defaultSaveSlotId();
    QString saveSlotDirectory(const QString& saveSlotId);
    QString saveDatabasePath(const QString& saveSlotId);

    GameBootstrapOptions createBootstrapOptionsForSaveSlot(
        const QString& saveSlotId,
        DatabaseOpenMode openMode);

    GameBootstrapOptions createDefaultSaveBootstrapOptions();
}
