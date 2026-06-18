#pragma once

#include <QObject>
#include <QList>
#include "../models/diskinfo.h"

class DiskManager : public QObject
{
    Q_OBJECT
public:
    explicit DiskManager(QObject *parent = nullptr);

    QList<DiskInfo> enumerateFixedDisks();
    DiskInfo        getDiskInfo(const QString &letter);
    bool            hasEnoughSpace(const QString &diskLetter, qint64 requiredBytes);
};

