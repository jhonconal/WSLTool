#pragma once

#include <QObject>
#include "../models/systeminfo.h"

class SystemDetector : public QObject
{
    Q_OBJECT
public:
    explicit SystemDetector(QObject *parent = nullptr);

    SystemInfo detect();

private:
    SystemInfo detectWindows();
    void       detectWsl(SystemInfo &info);
    bool       runProcess(const QString &program,
                          const QStringList &args,
                          QString &output,
                          int timeoutMs = 8000);
};

