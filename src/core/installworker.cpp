#include "installworker.h"
#include "wslmanager.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QDateTime>
#include <QThread>

InstallWorker::InstallWorker(const InstallConfig &config, QObject *parent)
    : QThread(parent)
    , m_config(config)
    , m_cancelRequested(false)
    , m_exported(false)
    , m_unregistered(false)
{
}

void InstallWorker::requestCancel()
{
    m_cancelRequested = true;
}

void InstallWorker::run()
{
    // 计算总步骤数
    int totalSteps = 1; // 仅下载安装
    if (m_config.createAccount) {
        totalSteps += 4; // 初始化、创建用户、设置密码、写入配置
    }
    if (!m_config.isDefaultPath) {
        totalSteps += 5; // 停止、导出、注销、导入、清理
    }

    int currentStep = 1;

    emitLog("=== WSL 发行版自动部署任务开始 ===");
    emitLog(QString("目标发行版: %1").arg(m_config.distroName));
    emitLog(m_config.isDefaultPath ? "部署模式: 默认路径安装" : QString("部署模式: 自定义路径安装 -> %1").arg(m_config.targetDirectory));
    if (m_config.createAccount) {
        emitLog(QString("配置账号: %1").arg(m_config.username));
    }
    emitLog("");

    // ---- 步骤 1: 下载安装 (wsl --install --no-launch) ----
    emitStep(currentStep++, totalSteps, "正在从应用商店下载并下载安装发行版...");
    emitLog("[提示] 首次下载可能较慢，请耐心等待控制台流日志...");
    if (m_cancelRequested) { emit finished(false, "用户已取消"); return; }
    if (!stepInstallDistro()) {
        emit finished(false, "下载安装发行版失败。请检查网络或系统 WSL 配置。");
        return;
    }

    // ---- 账户配置相关步骤 ----
    if (m_config.createAccount) {
        // 步骤 2: 初始化发行版 (解压 rootfs)
        emitStep(currentStep++, totalSteps, "正在初始化发行版系统环境...");
        emitLog("[提示] 该步骤在后台解压并注册发行版，通常需要 1~2 分钟，请稍候...");
        if (m_cancelRequested) { emit finished(false, "用户已取消"); return; }
        if (!stepInitialize()) {
            emit finished(false, "发行版环境初始化失败。");
            return;
        }

        // 步骤 3: 创建用户
        emitStep(currentStep++, totalSteps, QString("正在创建 Linux 用户账户: %1...").arg(m_config.username));
        if (m_cancelRequested) { emit finished(false, "用户已取消"); return; }
        if (!stepCreateUser()) {
            emit finished(false, "创建 Linux 用户账户失败。");
            return;
        }

        // 步骤 4: 设置密码
        emitStep(currentStep++, totalSteps, "正在设置账户密码...");
        if (m_cancelRequested) { emit finished(false, "用户已取消"); return; }
        if (!stepSetPassword()) {
            emit finished(false, "设置密码失败。");
            return;
        }

        // 步骤 5: 配置默认登录用户
        emitStep(currentStep++, totalSteps, "正在配置系统默认登录用户...");
        if (m_cancelRequested) { emit finished(false, "用户已取消"); return; }
        if (!stepSetDefaultUser()) {
            emitLog("[警告] 配置默认登录用户失败，后续您可能需要手动设置。");
        }
    }

    // ---- 自定义路径迁移步骤 ----
    if (!m_config.isDefaultPath) {
        // 临时备份路径
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        m_tempTarPath = QDir::toNativeSeparators(
            m_config.targetDirectory + "/" + m_config.distroName + "_temp_install_" + timestamp + ".tar");

        // 记录原始 DefaultUid
        int originalUid = -1;
        {
            QSettings lxss("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Lxss",
                            QSettings::NativeFormat);
            QStringList guids = lxss.childGroups();
            for (const QString &guid : guids) {
                lxss.beginGroup(guid);
                QString name = lxss.value("DistributionName", "").toString();
                int uid = lxss.value("DefaultUid", -1).toInt();
                lxss.endGroup();
                if (name.compare(m_config.distroName, Qt::CaseInsensitive) == 0) {
                    originalUid = uid;
                    break;
                }
            }
        }

        // 步骤 6: 停止发行版
        emitStep(currentStep++, totalSteps, "正在停止运行实例以准备迁移...");
        if (m_cancelRequested) { emit finished(false, "用户已取消"); return; }
        QString out;
        runProcess("wsl.exe", {"--terminate", m_config.distroName}, out, 15000, false);
        QThread::msleep(1000);

        // 步骤 7: 导出备份
        emitStep(currentStep++, totalSteps, "正在打包导出系统镜像...");
        if (m_cancelRequested) { emit finished(false, "用户已取消"); return; }
        if (!stepMigrate()) {
            emit finished(false, "自定义路径迁移失败，未能导出镜像。");
            return;
        }

        // 步骤 8: 注销默认路径
        emitStep(currentStep++, totalSteps, "正在清理默认路径的原实例...");
        if (m_cancelRequested) { emit finished(false, "用户已取消"); return; }
        runProcess("wsl.exe", {"--unregister", m_config.distroName}, out, 60000);
        m_unregistered = true;

        // 步骤 9: 导入至目标路径
        emitStep(currentStep++, totalSteps, QString("正在部署发行版至自定义目标路径: %1").arg(m_config.targetDirectory));
        QString importDir = m_config.targetDirectory + "/" + m_config.distroName;
        QDir().mkpath(importDir);
        bool importOk = runProcess("wsl.exe",
                                   {"--import", m_config.distroName, importDir, m_tempTarPath, "--version", "2"},
                                   out, 3600000);
        if (!importOk) {
            emit finished(false, "部署至目标路径失败！");
            return;
        }

        // 恢复 DefaultUid
        if (originalUid > 0) {
            QSettings lxss("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Lxss",
                            QSettings::NativeFormat);
            QStringList guids = lxss.childGroups();
            for (const QString &guid : guids) {
                lxss.beginGroup(guid);
                QString n = lxss.value("DistributionName", "").toString();
                lxss.endGroup();
                if (n.compare(m_config.distroName, Qt::CaseInsensitive) == 0) {
                    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Lxss\\" + guid,
                                  QSettings::NativeFormat);
                    reg.setValue("DefaultUid", originalUid);
                    emitLog(QString("  已恢复默认登录用户 UID: %1").arg(originalUid));
                    break;
                }
            }
        }

        // 步骤 10: 清理临时文件
        emitStep(currentStep++, totalSteps, "正在清理部署临时文件...");
        if (QFileInfo::exists(m_tempTarPath)) {
            QFile::remove(m_tempTarPath);
        }
    }

    emitLog("");
    emitLog("=== 自动部署成功完成！ ===");
    emit progressValue(100);
    emit finished(true, QString("发行版 <b>%1</b> 已成功下载并部署完毕！").arg(m_config.friendlyName));
}

bool InstallWorker::stepInstallDistro()
{
    QString out;
    return runProcess("wsl.exe", {"--install", m_config.distroName, "--no-launch"}, out, 1800000); // 30min max
}

bool InstallWorker::stepInitialize()
{
    QString out;
    // 运行一个最简指令，迫使 WSL 在后台把 rootfs 解压注册完毕
    return runProcess("wsl.exe", {"-d", m_config.distroName, "-u", "root", "--", "echo", "initialized"}, out, 300000);
}

bool InstallWorker::stepCreateUser()
{
    QString out;
    // 创建用户并加入 sudo 或 wheel 用户组
    QString cmd = QString(
        "useradd -m -s /bin/bash %1 2>/dev/null || adduser -D -s /bin/sh %2"
    ).arg(m_config.username, m_config.username);
    
    bool ok = runProcess("wsl.exe", {"-d", m_config.distroName, "-u", "root", "--", "sh", "-c", cmd}, out, 30000);
    if (!ok) return false;

    // 加入权限组
    QString groupCmd = QString(
        "usermod -aG sudo %1 2>/dev/null || usermod -aG wheel %2 2>/dev/null"
    ).arg(m_config.username, m_config.username);
    runProcess("wsl.exe", {"-d", m_config.distroName, "-u", "root", "--", "sh", "-c", groupCmd}, out, 30000);

    return true;
}

bool InstallWorker::stepSetPassword()
{
    // 执行 chpasswd 设置密码
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start("wsl.exe", {"-d", m_config.distroName, "-u", "root", "--", "sh", "-c",
                            QString("echo '%1:%2' | chpasswd").arg(m_config.username, m_config.password)});
    
    if (proc.waitForStarted(5000) && proc.waitForFinished(30000)) {
        return proc.exitCode() == 0;
    }
    return false;
}

bool InstallWorker::stepSetDefaultUser()
{
    // 1. 写入 /etc/wsl.conf (WSL 官方首选的、在 rootfs 中持久化默认用户的方法)
    QString out;
    QString cmd = QString("printf \"[user]\\ndefault = %1\\n\" > /etc/wsl.conf").arg(m_config.username);
    runProcess("wsl.exe", {"-d", m_config.distroName, "-u", "root", "--", "sh", "-c", cmd}, out, 15000);

    // 2. 同时写入 Windows 注册表中的 DefaultUid (双重保障)
    WslManager wm;
    return wm.setDefaultUser(m_config.distroName, m_config.username);
}

bool InstallWorker::stepMigrate()
{
    QDir dir(m_config.targetDirectory);
    if (!dir.exists() && !dir.mkpath(".")) {
        return false;
    }

    emitLog(QString("  正在将发行版打包至临时镜像: %1").arg(m_tempTarPath));
    QString out;
    bool ok = runProcess("wsl.exe", {"--export", m_config.distroName, m_tempTarPath}, out, 3600000);
    if (ok && QFileInfo::exists(m_tempTarPath)) {
        m_exported = true;
        return true;
    }
    return false;
}

bool InstallWorker::runProcess(const QString &program,
                               const QStringList &args,
                               QString &output,
                               int timeoutMs,
                               bool emitLogFlag)
{
    Q_UNUSED(timeoutMs);
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(program, args);

    if (!proc.waitForStarted(5000)) {
        output = "无法启动进程";
        return false;
    }

    // 实时读取控制台流输出并写入日志框
    while (!proc.waitForFinished(500)) {
        QByteArray chunk = proc.readAll();
        if (!chunk.isEmpty() && emitLogFlag) {
            // Windows CLI 默认用 Local8bit
            QString text = QString::fromLocal8Bit(chunk);
            emitLog(text);
        }

        if (m_cancelRequested) {
            proc.kill();
            return false;
        }
    }

    QByteArray raw = proc.readAllStandardOutput();
    if (!raw.isEmpty() && emitLogFlag) {
        emitLog(QString::fromLocal8Bit(raw));
    }
    output = QString::fromLocal8Bit(raw);

    return proc.exitCode() == 0;
}

void InstallWorker::emitStep(int step, int total, const QString &desc)
{
    emit stepChanged(step, total, desc);
    emitLog(QString("\n[步骤 %1/%2] %3").arg(step).arg(total).arg(desc));
}

void InstallWorker::emitLog(const QString &msg)
{
    emit logMessage(msg);
}
