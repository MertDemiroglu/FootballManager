#pragma once

#include"SaveSlotInfo.h"

#include"fm/core/GameBootstrapOptions.h"

#include<QList>
#include<QString>

class SaveSlotService {
public:
    QList<SaveSlotInfo> listSaveSlots() const;

    QString createUniqueSaveSlotId(const QString& saveNameOrManagerName) const;

    GameBootstrapOptions createNewSaveBootstrapOptions(
        const QString& saveSlotId,
        const QString& saveName) const;

    GameBootstrapOptions loadExistingSaveBootstrapOptions(const QString& saveSlotId) const;

    bool saveSlotExists(const QString& saveSlotId) const;
    bool deleteSaveSlot(const QString& saveSlotId, QString* errorMessage = nullptr) const;

    QString lastSaveSlotId() const;
    void rememberLastSaveSlot(const QString& saveSlotId) const;
    void clearLastSaveSlot() const;
};
