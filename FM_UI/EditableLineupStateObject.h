#pragma once

#include <QObject>

class EditableLineupStateObject : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(EditableLineupStateObject)

    Q_PROPERTY(bool hasLineup READ hasLineup NOTIFY changed)
    Q_PROPERTY(int teamId READ teamId NOTIFY changed)
    Q_PROPERTY(int formation READ formation NOTIFY changed)
    Q_PROPERTY(QString formationText READ formationText NOTIFY changed)
    Q_PROPERTY(int slotCount READ slotCount NOTIFY changed)
    Q_PROPERTY(int rosterCount READ rosterCount NOTIFY changed)
    Q_PROPERTY(int assignedCount READ assignedCount NOTIFY changed)
    Q_PROPERTY(bool isFull READ isFull NOTIFY changed)

public:
    explicit EditableLineupStateObject(QObject* parent = nullptr);

    bool hasLineup() const;
    int teamId() const;
    int formation() const;
    QString formationText() const;
    int slotCount() const;
    int rosterCount() const;
    int assignedCount() const;
    bool isFull() const;

    void setFromValues(bool hasLineup,
        int teamId,
        int formation,
        const QString& formationText,
        int slotCount,
        int rosterCount,
        int assignedCount,
        bool isFull);
    void clear();

signals:
    void changed();

private:
    bool hasLineupValue = false;
    int teamIdValue = 0;
    int formationValue = 0;
    QString formationTextValue;
    int slotCountValue = 0;
    int rosterCountValue = 0;
    int assignedCountValue = 0;
    bool isFullValue = false;
};
