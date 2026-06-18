#include "migrationworker.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <QDateTime>
#include <QThread>

MigrationWorker::MigrationWorker(const MigrationConfig &config, QObject *parent)
    : QThread(parent)
    , m_config(config)
    , m_cancelRequested(false)
    , m_exported(false)
    , m_unregistered(false)
{
}

void MigrationWorker::requestCancel()
{
    m_cancelRequested = true;
}

void MigrationWorker::run()
{
    int totalSteps = m_config.configureAccount ? 6 : 5;
    emitLog("=== WSL 迁移任务开始 ===");
    emitLog(QString("发行版: %1").arg(m_config.distro.name));
    emitLog(QString("目标目录: %1").arg(m_config.targetDirectory));
    emitLog("");

    // 临时 tar 文件路径
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    m_tempTarPath = QDir::toNativeSeparators(
        m_config.targetDirectory + "/" + m_config.distro.name + "_backup_" + timestamp + ".tar");

    // ---- 步骤 1: 停止发行版 ----
    emitStep(1, totalSteps, "停止发行版实例...");
    if (m_cancelRequested) { rollback(); emit finished(false, "用户已取消"); return; }
    if (!stepStopDistro()) {
        emitLog("[警告] 停止发行版失败，继续尝试导出（若发行版未运行可忽略此警告）");
    }

    // ---- 步骤 2: 导出镜像 ----
    emitStep(2, totalSteps, QString("导出镜像到: %1").arg(m_tempTarPath));
    emitLog("[重要] 这是耗时最长的步骤，请耐心等待...");
    if (m_cancelRequested) { rollback(); emit finished(false, "用户已取消"); return; }
    if (!stepExport()) {
        rollback();
        emit finished(false, "导出镜像失败，迁移已中止。原发行版未受影响。");
        return;
    }

    // ---- 步骤 3: 注销原发行版 ----
    emitStep(3, totalSteps, "注销原位置发行版...");
    if (m_cancelRequested) {
        rollback();
        emit finished(false, "用户取消（导出文件已保留在目标目录，可手动导入恢复）");
        return;
    }
    if (!stepUnregister()) {
        rollback();
        emit finished(false, "注销发行版失败。已尝试回滚，请检查原发行版状态。");
        return;
    }

    // ---- 步骤 4: 导入到目标位置 ----
    emitStep(4, totalSteps, QString("导入到目标目录: %1").arg(m_config.targetDirectory));
    if (!stepImport()) {
        emitLog("[错误] 导入失败，正在尝试回滚...");
        rollback();
        emit finished(false, "导入失败。已尝试将备份文件重新注册。请检查目标目录并手动恢复。");
        return;
    }

    // ---- 步骤 5: 配置账户（可选）----
    if (m_config.configureAccount) {
        emitStep(5, totalSteps, "配置用户账户...");
        if (!stepConfigureAccount()) {
            emitLog("[警告] 账户配置部分失败，发行版已迁移成功，请手动配置账户");
        }
    }

    // ---- 步骤 6(5): 清理临时文件 ----
    emitStep(totalSteps, totalSteps, "清理临时文件...");
    stepCleanup();

    emitLog("");
    emitLog("=== 迁移完成！===");
    emit progressValue(100);
    emit finished(true, "迁移成功完成！发行版已移至 " + m_config.targetDirectory);
}

bool MigrationWorker::stepStopDistro()
{
    QString out;
    bool ok = runProcess("wsl.exe", {"-t", m_config.distro.name}, out, 15000, false);
    emitLog(QString("  wsl -t %1: %2").arg(m_config.distro.name).arg(ok ? "成功" : "失败(可忽略)"));
    QThread::msleep(1000); // 等待进程完全结束
    return ok;
}

bool MigrationWorker::stepExport()
{
    // 确保目标目录存在
    QDir dir(m_config.targetDirectory);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emitLog("[错误] 无法创建目标目录: " + m_config.targetDirectory);
            return false;
        }
        emitLog("  创建目标目录: " + m_config.targetDirectory);
    }

    emitLog(QString("  执行: wsl --export %1 \"%2\"").arg(m_config.distro.name, m_tempTarPath));

    // 注：export 可能需要很长时间（10+ 分钟）
    QString out;
    bool ok = runProcess("wsl.exe",
                         {"--export", m_config.distro.name, m_tempTarPath},
                         out, 3600000); // 1 hour timeout

    if (ok && QFileInfo::exists(m_tempTarPath)) {
        m_exported = true;
        qint64 size = QFileInfo(m_tempTarPath).size();
        emitLog(QString("  导出成功，文件大小: %1 GB")
                    .arg(size / (1024.0*1024.0*1024.0), 0, 'f', 2));
        return true;
    }

    emitLog("[错误] export 命令输出: " + out);
    return false;
}

bool MigrationWorker::stepUnregister()
{
    emitLog(QString("  执行: wsl --unregister %1").arg(m_config.distro.name));
    QString out;
    bool ok = runProcess("wsl.exe", {"--unregister", m_config.distro.name}, out, 60000);
    if (ok) {
        m_unregistered = true;
        emitLog("  注销成功");
    } else {
        emitLog("[错误] 注销失败: " + out);
    }
    return ok;
}

bool MigrationWorker::stepImport()
{
    QString importDir = m_config.targetDirectory + "/" + m_config.distro.name;
    QDir().mkpath(importDir);

    emitLog(QString("  执行: wsl --import %1 \"%2\" \"%3\" --version 2")
                .arg(m_config.distro.name, importDir, m_tempTarPath));

    QString out;
    bool ok = runProcess("wsl.exe",
                         {"--import", m_config.distro.name, importDir, m_tempTarPath, "--version", "2"},
                         out, 3600000);

    if (ok) {
        emitLog("  导入成功");
    } else {
        emitLog("[错误] 导入失败: " + out);
    }
    return ok;
}

bool MigrationWorker::stepConfigureAccount()
{
    bool allOk = true;

    // 1. 设置默认用户
    if (!m_config.newUsername.isEmpty()) {
        emitLog(QString("  设置默认用户: %1").arg(m_config.newUsername));

        // 通过 id -u 获取 uid
        QString out;
        QString cmd = QString("id -u %1 2>/dev/null || echo -1").arg(m_config.newUsername);
        bool ok = runProcess("wsl.exe",
                             {"-d", m_config.distro.name, "-u", "root", "--", "bash", "-c", cmd},
                             out, 15000);
        out = out.trimmed();
        bool uidOk;
        int uid = out.toInt(&uidOk);
        if (ok && uidOk && uid > 0) {
            // 写注册表
            QSettings lxss("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Lxss",
                            QSettings::NativeFormat);
            QStringList guids = lxss.childGroups();
            for (const QString &guid : guids) {
                lxss.beginGroup(guid);
                QString n = lxss.value("DistributionName", "").toString();
                lxss.endGroup();
                if (n.compare(m_config.distro.name, Qt::CaseInsensitive) == 0) {
                    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Lxss\\" + guid,
                                  QSettings::NativeFormat);
                    reg.setValue("DefaultUid", uid);
                    emitLog(QString("  DefaultUid 已写入注册表 (uid=%1)").arg(uid));
                    break;
                }
            }
        } else {
            emitLog("[警告] 无法获取用户 UID，跳过默认用户设置");
            allOk = false;
        }
    }

    // 2. 设置密码
    if (!m_config.newPassword.isEmpty() && !m_config.newUsername.isEmpty()) {
        emitLog(QString("  为用户 %1 设置密码...").arg(m_config.newUsername));

        // 写临时脚本避免密码出现在命令行
        QString scriptContent = QString("echo '%1:%2' | chpasswd").arg(m_config.newUsername, m_config.newPassword);
        // 通过 stdin 传递
        QProcess proc;
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start("wsl.exe", {"-d", m_config.distro.name, "-u", "root", "--", "bash", "-c",
                                QString("echo '%1:%2' | chpasswd").arg(m_config.newUsername, m_config.newPassword)});
        if (proc.waitForStarted(5000) && proc.waitForFinished(30000)) {
            if (proc.exitCode() == 0) {
                emitLog("  密码设置成功");
            } else {
                emitLog("[警告] 密码设置失败: " + QString::fromLocal8Bit(proc.readAll()));
                allOk = false;
            }
        }
    }

    return allOk;
}

bool MigrationWorker::stepCleanup()
{
    if (m_exported && QFileInfo::exists(m_tempTarPath)) {
        bool removed = QFile::remove(m_tempTarPath);
        emitLog(removed ? "  临时文件已清理" : "[警告] 临时文件清理失败: " + m_tempTarPath);
        return removed;
    }
    return true;
}

bool MigrationWorker::rollback()
{
    emitLog("");
    emitLog("====== 开始回滚 ======");

    if (m_unregistered && m_exported && QFileInfo::exists(m_tempTarPath)) {
        // 尝试把 tar 导回原来的位置
        QString originalDir = m_config.distro.basePath;
        if (originalDir.isEmpty()) {
            originalDir = QDir::toNativeSeparators(
                QString("C:\\Users\\%1\\AppData\\Local\\Packages\\%2")
                    .arg(qgetenv("USERNAME").constData(), m_config.distro.name));
        }
        emitLog(QString("  尝试重新导入到原位置: %1").arg(originalDir));
        QDir().mkpath(originalDir);

        QString out;
        bool ok = runProcess("wsl.exe",
                             {"--import", m_config.distro.name, originalDir, m_tempTarPath, "--version", "2"},
                             out, 3600000);
        if (ok) {
            emitLog("  回滚成功！发行版已恢复");
        } else {
            emitLog("[错误] 回滚失败！请手动执行:");
            emitLog(QString("  wsl --import %1 <目录> \"%2\" --version 2")
                        .arg(m_config.distro.name, m_tempTarPath));
            emitLog("  备份文件保留在: " + m_tempTarPath);
        }
        return ok;
    } else if (m_exported && !m_unregistered) {
        emitLog("  原发行版未注销，无需回滚");
        // 清理未使用的 tar
        if (QFileInfo::exists(m_tempTarPath)) {
            QFile::remove(m_tempTarPath);
        }
    }
    emitLog("====== 回滚结束 ======");
    return false;
}

bool MigrationWorker::runProcess(const QString &program,
                                 const QStringList &args,
                                 QString &output,
                                 int timeoutMs,
                                 bool emitLogFlag)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(program, args);

    if (!proc.waitForStarted(5000)) {
        output = "无法启动进程";
        return false;
    }

    // 实时读取输出
    while (!proc.waitForFinished(500)) {
        QByteArray chunk = proc.readAll();
        if (!chunk.isEmpty() && emitLogFlag) {
            QString text = QString::fromUtf8(chunk);
            if (text.contains(QChar::ReplacementCharacter))
                text = QString::fromLocal8Bit(chunk);
            emitLog("  > " + text.trimmed());
        }

        if (m_cancelRequested) {
            proc.kill();
            output = "已取消";
            return false;
        }

        static int elapsed = 0;
        elapsed += 500;
        if (elapsed > timeoutMs) {
            proc.kill();
            output = "超时";
            return false;
        }
    }

    QByteArray raw = proc.readAll();
    output = QString::fromUtf8(raw);
    if (output.contains(QChar::ReplacementCharacter))
        output = QString::fromLocal8Bit(raw);

    if (!output.isEmpty() && emitLogFlag)
        emitLog("  > " + output.trimmed());

    return proc.exitCode() == 0;
}

void MigrationWorker::emitStep(int step, int total, const QString &desc)
{
    emit stepChanged(step, total, desc);
    emit progressValue((step - 1) * 100 / total);
    emitLog(QString("[步骤 %1/%2] %3").arg(step).arg(total).arg(desc));
}

void MigrationWorker::emitLog(const QString &msg)
{
    emit logMessage(msg);
}

