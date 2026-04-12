#pragma once

#include <QObject>

class DashboardNextMatchObject : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(DashboardNextMatchObject)

    Q_PROPERTY(bool hasNextMatch READ hasNextMatch NOTIFY changed)
    Q_PROPERTY(QString dateText READ dateText NOTIFY changed)
    Q_PROPERTY(QString homeTeamName READ homeTeamName NOTIFY changed)
    Q_PROPERTY(QString awayTeamName READ awayTeamName NOTIFY changed)
    Q_PROPERTY(bool isHome READ isHome NOTIFY changed)
    Q_PROPERTY(int matchweek READ matchweek NOTIFY changed)

public:
    explicit DashboardNextMatchObject(QObject* parent = nullptr);

    bool hasNextMatch() const;
    QString dateText() const;
    QString homeTeamName() const;
    QString awayTeamName() const;
    bool isHome() const;
    int matchweek() const;

    void setFromValues(const QString& dateText, const QString& homeTeamName, const QString& awayTeamName, bool isHome, int matchweek);
    void clear();

signals:
    void changed();

private:
    bool hasNextMatchValue = false;
    QString dateTextValue;
    QString homeTeamNameValue;
    QString awayTeamNameValue;
    bool isHomeValue = false;
    int matchweekValue = 0;
};
