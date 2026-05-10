#pragma once

#include <QObject>

class DashboardNextMatchObject : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(DashboardNextMatchObject)

    Q_PROPERTY(bool hasNextMatch READ hasNextMatch NOTIFY changed)
    Q_PROPERTY(QString dateText READ dateText NOTIFY changed)
    Q_PROPERTY(QString homeTeamName READ homeTeamName NOTIFY changed)
    Q_PROPERTY(QString homePrimaryColor READ homePrimaryColor NOTIFY changed)
    Q_PROPERTY(QString homeSecondaryColor READ homeSecondaryColor NOTIFY changed)
    Q_PROPERTY(QString homeTextColor READ homeTextColor NOTIFY changed)
    Q_PROPERTY(QString awayTeamName READ awayTeamName NOTIFY changed)
    Q_PROPERTY(QString awayPrimaryColor READ awayPrimaryColor NOTIFY changed)
    Q_PROPERTY(QString awaySecondaryColor READ awaySecondaryColor NOTIFY changed)
    Q_PROPERTY(QString awayTextColor READ awayTextColor NOTIFY changed)
    Q_PROPERTY(bool isHome READ isHome NOTIFY changed)
    Q_PROPERTY(int matchweek READ matchweek NOTIFY changed)

public:
    explicit DashboardNextMatchObject(QObject* parent = nullptr);

    bool hasNextMatch() const;
    QString dateText() const;
    QString homeTeamName() const;
    QString homePrimaryColor() const;
    QString homeSecondaryColor() const;
    QString homeTextColor() const;
    QString awayTeamName() const;
    QString awayPrimaryColor() const;
    QString awaySecondaryColor() const;
    QString awayTextColor() const;
    bool isHome() const;
    int matchweek() const;

    void setFromValues(const QString& dateText,
        const QString& homeTeamName,
        const QString& homePrimaryColor,
        const QString& homeSecondaryColor,
        const QString& homeTextColor,
        const QString& awayTeamName,
        const QString& awayPrimaryColor,
        const QString& awaySecondaryColor,
        const QString& awayTextColor,
        bool isHome,
        int matchweek);
    void clear();

signals:
    void changed();

private:
    bool hasNextMatchValue = false;
    QString dateTextValue;
    QString homeTeamNameValue;
    QString homePrimaryColorValue;
    QString homeSecondaryColorValue;
    QString homeTextColorValue;
    QString awayTeamNameValue;
    QString awayPrimaryColorValue;
    QString awaySecondaryColorValue;
    QString awayTextColorValue;
    bool isHomeValue = false;
    int matchweekValue = 0;
};
