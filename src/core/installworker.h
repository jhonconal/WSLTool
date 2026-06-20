#pragma once
#include <QThread>
#include <QString>
#include "../models/wsldistribution.h"

struct InstallConfig {
    QString distroName;
    QString friendlyName;
    bool    isDefaultPath;
    QString targetDirectory; // Custom path if not default

    bool    createAccount;
    QString username;
    QString password;
};

class InstallWorker : public QThread
{
    Q_OBJECT
public:
    explicit InstallWorker(const InstallConfig &config, QObject *parent = nullptr);

    void run() override;
    void requestCancel();

signals:
    void stepChanged(int step, int total, const QString &description);
    void logMessage(const QString &msg);
    void progressValue(int percent);
    void finished(bool success, const QString &message);

private:
    InstallConfig m_config;
    bool          m_cancelRequested;
    QString       m_tempTarPath;
    bool          m_exported;
    bool          m_unregistered;

    bool stepInstallDistro();
    bool stepInitialize();
    bool stepCreateUser();
    bool stepSetPassword();
    bool stepSetDefaultUser();
    bool stepMigrate();

    // Helper process runner that streams stdout to log
    bool runProcess(const QString &program,
                    const QStringList &args,
                    QString &output,
                    int timeoutMs = 600000,
                    bool emitLog = true);

    void emitStep(int step, int total, const QString &desc);
    void emitLog(const QString &msg);
};
