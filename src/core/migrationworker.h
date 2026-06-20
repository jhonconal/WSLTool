#pragma once

#include <QThread>
#include <QString>
#include "../models/wsldistribution.h"

struct MigrationConfig {
    WslDistribution distro;
    QString         targetDirectory;   // 目标目录，e.g. "D:\WSL\Ubuntu"
    QString         newUsername;       // 账户名（可为空，不修改）
    QString         newPassword;       // 密码（可为空，不修改）
    bool            configureAccount;  // 是否配置账户
};

class MigrationWorker : public QThread
{
    Q_OBJECT
public:
    explicit MigrationWorker(const MigrationConfig &config, QObject *parent = nullptr);

    void run() override;
    void requestCancel();

signals:
    void stepChanged(int step, int total, const QString &description);
    void logMessage(const QString &msg);
    void progressValue(int percent);
    void finished(bool success, const QString &message);

private:
    MigrationConfig m_config;
    bool            m_cancelRequested;
    QString         m_tempTarPath;
    bool            m_exported;
    bool            m_unregistered;
    int             m_originalUid;

    bool stepStopDistro();
    bool stepExport();
    bool stepUnregister();
    bool stepImport();
    bool stepConfigureAccount();
    bool stepCleanup();

    bool rollback();

    bool runProcess(const QString &program,
                    const QStringList &args,
                    QString &output,
                    int timeoutMs = 600000,   // 10min default for export
                    bool emitLog = true);

    void emitStep(int step, int total, const QString &desc);
    void emitLog(const QString &msg);
};

