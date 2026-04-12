#pragma once

#include <QObject>

#include "DashboardNextMatchObject.h"
#include "DashboardStandingsRowObject.h"
#include "DashboardTeamStatsObject.h"

class DashboardStateObject : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(DashboardStateObject)

    Q_PROPERTY(bool hasData READ hasData NOTIFY changed)
    Q_PROPERTY(QString selectedTeamName READ selectedTeamName NOTIFY changed)
    Q_PROPERTY(QString currentDateText READ currentDateText NOTIFY changed)
    Q_PROPERTY(QString gameStateText READ gameStateText NOTIFY changed)
    Q_PROPERTY(QString managerName READ managerName NOTIFY changed)
    Q_PROPERTY(QString recentForm READ recentForm NOTIFY changed)
    Q_PROPERTY(DashboardNextMatchObject* nextMatch READ nextMatch CONSTANT)
    Q_PROPERTY(DashboardTeamStatsObject* teamStats READ teamStats CONSTANT)
    Q_PROPERTY(DashboardStandingsRowObject* standingsRow READ standingsRow CONSTANT)

public:
    explicit DashboardStateObject(QObject* parent = nullptr);

    bool hasData() const;
    QString selectedTeamName() const;
    QString currentDateText() const;
    QString gameStateText() const;
    QString managerName() const;
    QString recentForm() const;

    DashboardNextMatchObject* nextMatch() const;
    DashboardTeamStatsObject* teamStats() const;
    DashboardStandingsRowObject* standingsRow() const;

    void setRootValues(bool hasData,
        const QString& selectedTeamName,
        const QString& currentDateText,
        const QString& gameStateText,
        const QString& managerName,
        const QString& recentForm);
    void clear();

signals:
    void changed();

private:
    bool hasDataValue = false;
    QString selectedTeamNameValue;
    QString currentDateTextValue;
    QString gameStateTextValue;
    QString managerNameValue;
    QString recentFormValue;
    DashboardNextMatchObject nextMatchObject;
    DashboardTeamStatsObject teamStatsObject;
    DashboardStandingsRowObject standingsRowObject;
};
