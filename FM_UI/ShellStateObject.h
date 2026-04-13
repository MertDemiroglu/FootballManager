#pragma once

#include <QObject>

class ShellStateObject : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(ShellStateObject)

    Q_PROPERTY(bool hasStartedGame READ hasStartedGame NOTIFY changed)
    Q_PROPERTY(QString selectedTeamName READ selectedTeamName NOTIFY changed)
    Q_PROPERTY(QString currentDateText READ currentDateText NOTIFY changed)
    Q_PROPERTY(QString currentStateText READ currentStateText NOTIFY changed)
    Q_PROPERTY(QString managerName READ managerName NOTIFY changed)
    Q_PROPERTY(bool timePaused READ timePaused NOTIFY changed)

public:
    explicit ShellStateObject(QObject* parent = nullptr);

    bool hasStartedGame() const;
    QString selectedTeamName() const;
    QString currentDateText() const;
    QString currentStateText() const;
    QString managerName() const;
    bool timePaused() const;

    void setFromValues(bool hasStartedGame,
        const QString& selectedTeamName,
        const QString& currentDateText,
        const QString& currentStateText,
        const QString& managerName,
        bool timePaused);
    void clear();

signals:
    void changed();

private:
    bool hasStartedGameValue = false;
    QString selectedTeamNameValue;
    QString currentDateTextValue;
    QString currentStateTextValue;
    QString managerNameValue;
    bool timePausedValue = true;
};
