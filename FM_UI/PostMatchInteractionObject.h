#pragma once

#include <QObject>
#include <QVariantList>

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
    Q_PROPERTY(QString homePrimaryColor READ homePrimaryColor NOTIFY changed)
    Q_PROPERTY(QString homeSecondaryColor READ homeSecondaryColor NOTIFY changed)
    Q_PROPERTY(QString homeTextColor READ homeTextColor NOTIFY changed)
    Q_PROPERTY(QString awayPrimaryColor READ awayPrimaryColor NOTIFY changed)
    Q_PROPERTY(QString awaySecondaryColor READ awaySecondaryColor NOTIFY changed)
    Q_PROPERTY(QString awayTextColor READ awayTextColor NOTIFY changed)
    Q_PROPERTY(int homeGoals READ homeGoals NOTIFY changed)
    Q_PROPERTY(int awayGoals READ awayGoals NOTIFY changed)
    Q_PROPERTY(QString scoreLine READ scoreLine NOTIFY changed)
    Q_PROPERTY(QString scorerSummary READ scorerSummary NOTIFY changed)
    Q_PROPERTY(QString assistSummary READ assistSummary NOTIFY changed)
    Q_PROPERTY(QString cardSummary READ cardSummary NOTIFY changed)
    Q_PROPERTY(QString homeCoachName READ homeCoachName NOTIFY changed)
    Q_PROPERTY(QString awayCoachName READ awayCoachName NOTIFY changed)
    Q_PROPERTY(QString homeFormationText READ homeFormationText NOTIFY changed)
    Q_PROPERTY(QString awayFormationText READ awayFormationText NOTIFY changed)
    Q_PROPERTY(QString homeAverageOverallText READ homeAverageOverallText NOTIFY changed)
    Q_PROPERTY(QString awayAverageOverallText READ awayAverageOverallText NOTIFY changed)
    Q_PROPERTY(QVariantList homeLineup READ homeLineup NOTIFY changed)
    Q_PROPERTY(QVariantList awayLineup READ awayLineup NOTIFY changed)
    Q_PROPERTY(QVariantList statRows READ statRows NOTIFY changed)

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
    QString homePrimaryColor() const;
    QString homeSecondaryColor() const;
    QString homeTextColor() const;
    QString awayPrimaryColor() const;
    QString awaySecondaryColor() const;
    QString awayTextColor() const;
    int homeGoals() const;
    int awayGoals() const;
    QString scoreLine() const;
    QString scorerSummary() const;
    QString assistSummary() const;
    QString cardSummary() const;
    QString homeCoachName() const;
    QString awayCoachName() const;
    QString homeFormationText() const;
    QString awayFormationText() const;
    QString homeAverageOverallText() const;
    QString awayAverageOverallText() const;
    QVariantList homeLineup() const;
    QVariantList awayLineup() const;
    QVariantList statRows() const;

    void setFromValues(qulonglong matchId,
        const QString& dateText,
        int matchweek,
        int homeTeamId,
        int awayTeamId,
        const QString& homeTeamName,
        const QString& awayTeamName,
        const QString& homePrimaryColor,
        const QString& homeSecondaryColor,
        const QString& homeTextColor,
        const QString& awayPrimaryColor,
        const QString& awaySecondaryColor,
        const QString& awayTextColor,
        int homeGoals,
        int awayGoals,
        const QString& scorerSummary,
        const QString& assistSummary,
        const QString& cardSummary,
        const QString& homeCoachName,
        const QString& awayCoachName,
        const QString& homeFormationText,
        const QString& awayFormationText,
        const QString& homeAverageOverallText,
        const QString& awayAverageOverallText,
        const QVariantList& homeLineup,
        const QVariantList& awayLineup,
        const QVariantList& statRows);
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
    QString homePrimaryColorValue = QStringLiteral("#22c55e");
    QString homeSecondaryColorValue = QStringLiteral("#0f172a");
    QString homeTextColorValue = QStringLiteral("#f8fafc");
    QString awayPrimaryColorValue = QStringLiteral("#22c55e");
    QString awaySecondaryColorValue = QStringLiteral("#0f172a");
    QString awayTextColorValue = QStringLiteral("#f8fafc");
    int homeGoalsValue = 0;
    int awayGoalsValue = 0;
    QString scorerSummaryValue;
    QString assistSummaryValue;
    QString cardSummaryValue;
    QString homeCoachNameValue;
    QString awayCoachNameValue;
    QString homeFormationTextValue;
    QString awayFormationTextValue;
    QString homeAverageOverallTextValue = QStringLiteral("--");
    QString awayAverageOverallTextValue = QStringLiteral("--");
    QVariantList homeLineupValue;
    QVariantList awayLineupValue;
    QVariantList statRowsValue;
};
