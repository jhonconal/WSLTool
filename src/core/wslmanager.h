#pragma once

#include <QObject>
#include <QList>
#include "../models/wsldistribution.h"

class WslManager : public QObject
{
    Q_OBJECT
public:
    explicit WslManager(QObject *parent = nullptr);

    QList<WslDistribution> enumerateDistributions();
    bool terminateDistribution(const QString &name);
    bool setDefaultUser(const QString &distroName, const QString &userName);

private:
    QList<WslDistribution> fromRegistry();
    void mergeCommandLineInfo(QList<WslDistribution> &list);
    bool runProcess(const QString &program,
                    const QStringList &args,
                    QString &output,
                    int timeoutMs = 30000);
    QString detectDistroDisplayName(const QString &regName);
    QString findVhdxPath(const QString &basePath);
    qint64  getFileSize(const QString &path);
};

