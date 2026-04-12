#pragma once

#include <QObject>

class DashboardStandingsRowObject : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(DashboardStandingsRowObject)

    Q_PROPERTY(bool hasData READ hasData NOTIFY changed)
    Q_PROPERTY(int position READ position NOTIFY changed)
    Q_PROPERTY(QString teamName READ teamName NOTIFY changed)
    Q_PROPERTY(int played READ played NOTIFY changed)
    Q_PROPERTY(int wins READ wins NOTIFY changed)
    Q_PROPERTY(int draws READ draws NOTIFY changed)
    Q_PROPERTY(int losses READ losses NOTIFY changed)
    Q_PROPERTY(int goalDifference READ goalDifference NOTIFY changed)
    Q_PROPERTY(int points READ points NOTIFY changed)

public:
    explicit DashboardStandingsRowObject(QObject* parent = nullptr);

    bool hasData() const;
    int position() const;
    QString teamName() const;
    int played() const;
    int wins() const;
    int draws() const;
    int losses() const;
    int goalDifference() const;
    int points() const;

    void setFromValues(int position, const QString& teamName, int played, int wins, int draws, int losses, int goalDifference, int points);
    void clear();

signals:
    void changed();

private:
    bool hasDataValue = false;
    int positionValue = 0;
    QString teamNameValue;
    int playedValue = 0;
    int winsValue = 0;
    int drawsValue = 0;
    int lossesValue = 0;
    int goalDifferenceValue = 0;
    int pointsValue = 0;
};
