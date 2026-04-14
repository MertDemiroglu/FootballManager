#pragma once

#include <QObject>

class PreMatchInteractionObject : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(PreMatchInteractionObject)

    Q_PROPERTY(bool hasData READ hasData NOTIFY changed)
    Q_PROPERTY(qulonglong matchId READ matchId NOTIFY changed)
    Q_PROPERTY(QString dateText READ dateText NOTIFY changed)
    Q_PROPERTY(int matchweek READ matchweek NOTIFY changed)
    Q_PROPERTY(int homeTeamId READ homeTeamId NOTIFY changed)
    Q_PROPERTY(int awayTeamId READ awayTeamId NOTIFY changed)
    Q_PROPERTY(QString homeTeamName READ homeTeamName NOTIFY changed)
    Q_PROPERTY(QString awayTeamName READ awayTeamName NOTIFY changed)
    Q_PROPERTY(QString fixtureLabel READ fixtureLabel NOTIFY changed)

public:
    explicit PreMatchInteractionObject(QObject* parent = nullptr);

    bool hasData() const;
    qulonglong matchId() const;
    QString dateText() const;
    int matchweek() const;
    int homeTeamId() const;
    int awayTeamId() const;
    QString homeTeamName() const;
    QString awayTeamName() const;
    QString fixtureLabel() const;

    void setFromValues(qulonglong matchId,
        const QString& dateText,
        int matchweek,
        int homeTeamId,
        int awayTeamId,
        const QString& homeTeamName,
        const QString& awayTeamName);
    void clear();

signals:
    void changed();

private:
    bool hasDataValue = false;
    qulonglong matchIdValue = 0;
    QString dateTextValue;
    int matchweekValue = 0;
    int homeTeamIdValue = 0;
    int awayTeamIdValue = 0;
    QString homeTeamNameValue;
    QString awayTeamNameValue;
};
