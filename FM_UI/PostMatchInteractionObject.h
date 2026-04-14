#pragma once

#include <QObject>

class PostMatchInteractionObject : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(PostMatchInteractionObject)

    Q_PROPERTY(bool hasData READ hasData NOTIFY changed)
    Q_PROPERTY(qulonglong matchId READ matchId NOTIFY changed)
    Q_PROPERTY(QString dateText READ dateText NOTIFY changed)
    Q_PROPERTY(int matchweek READ matchweek NOTIFY changed)
    Q_PROPERTY(int homeTeamId READ homeTeamId NOTIFY changed)
    Q_PROPERTY(int awayTeamId READ awayTeamId NOTIFY changed)
    Q_PROPERTY(QString homeTeamName READ homeTeamName NOTIFY changed)
    Q_PROPERTY(QString awayTeamName READ awayTeamName NOTIFY changed)
    Q_PROPERTY(int homeGoals READ homeGoals NOTIFY changed)
    Q_PROPERTY(int awayGoals READ awayGoals NOTIFY changed)
    Q_PROPERTY(QString scoreLine READ scoreLine NOTIFY changed)
    Q_PROPERTY(QString scorerSummary READ scorerSummary NOTIFY changed)
    Q_PROPERTY(QString assistSummary READ assistSummary NOTIFY changed)
    Q_PROPERTY(QString cardSummary READ cardSummary NOTIFY changed)

public:
    explicit PostMatchInteractionObject(QObject* parent = nullptr);

    bool hasData() const;
    qulonglong matchId() const;
    QString dateText() const;
    int matchweek() const;
    int homeTeamId() const;
    int awayTeamId() const;
    QString homeTeamName() const;
    QString awayTeamName() const;
    int homeGoals() const;
    int awayGoals() const;
    QString scoreLine() const;
    QString scorerSummary() const;
    QString assistSummary() const;
    QString cardSummary() const;

    void setFromValues(qulonglong matchId,
        const QString& dateText,
        int matchweek,
        int homeTeamId,
        int awayTeamId,
        const QString& homeTeamName,
        const QString& awayTeamName,
        int homeGoals,
        int awayGoals,
        const QString& scorerSummary,
        const QString& assistSummary,
        const QString& cardSummary);
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
    int homeGoalsValue = 0;
    int awayGoalsValue = 0;
    QString scorerSummaryValue;
    QString assistSummaryValue;
    QString cardSummaryValue;
};
