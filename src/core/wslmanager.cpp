#include "wslmanager.h"
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>

WslManager::WslManager(QObject *parent) : QObject(parent) {}

QList<WslDistribution> WslManager::enumerateDistributions()
{
    QList<WslDistribution> list = fromRegistry();
    mergeCommandLineInfo(list);
    return list;
}

QList<WslDistribution> WslManager::fromRegistry()
{
    QList<WslDistribution> list;

    QSettings lxss(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Lxss",
        QSettings::NativeFormat);

    // 获取默认发行版 GUID
    QString defaultGuid = lxss.value("DefaultDistribution", "").toString().toLower();

    QStringList guids = lxss.childGroups();
    for (const QString &guid : guids) {
        lxss.beginGroup(guid);

        WslDistribution d;
        d.registryGuid = guid;
        d.name         = lxss.value("DistributionName", "").toString();
        d.basePath     = lxss.value("BasePath", "").toString();
        d.isDefault    = (guid.toLower() == defaultGuid);

        int ver        = lxss.value("Version", 1).toInt();
        d.version      = (ver == 2) ? WslVersion::WSL2 : WslVersion::WSL1;

        int state      = lxss.value("State", 1).toInt();
        // State: 1=Stopped, 3=Running
        d.state        = (state == 3) ? DistroState::Running : DistroState::Stopped;

        lxss.endGroup();

        if (d.name.isEmpty()) continue;

        // 规范化路径（注册表有时用 \\?\ 前缀）
        if (d.basePath.startsWith("\\\\?\\"))
            d.basePath = d.basePath.mid(4);

        // 确定所在盘符
        if (!d.basePath.isEmpty() && d.basePath.length() >= 2) {
            d.diskLetter = d.basePath.left(1).toUpper();
        }
        d.isOnCDrive = (d.diskLetter.toUpper() == "C");

        // 查找 vhdx 文件
        d.vhdxPath = findVhdxPath(d.basePath);
        d.sizeBytes = d.vhdxPath.isEmpty() ? -1 : getFileSize(d.vhdxPath);

        d.displayName = detectDistroDisplayName(d.name);

        list.append(d);
    }

    return list;
}

void WslManager::mergeCommandLineInfo(QList<WslDistribution> &list)
{
    // wsl -l -v 输出格式:
    //   NAME            STATE           VERSION
    // * Ubuntu-22.04    Stopped         2
    //   Debian          Running         2
    QString out;
    if (!runProcess("wsl.exe", {"-l", "-v"}, out, 10000)) return;

    // 解析每行
    QStringList lines = out.split('\n');
    for (const QString &rawLine : lines) {
        // 去掉 BOM 和前导 * 空格
        QString line = rawLine;
        line.remove(QChar(0xFEFF)); // BOM
        bool isDefault = line.startsWith("*");
        line = line.trimmed().replace("*", "").simplified();

        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() < 3) continue;

        QString name    = parts[0];
        QString state   = parts[1].toLower();
        int     version = parts[2].toInt();

        // 在已有列表中匹配
        for (auto &d : list) {
            if (d.name.compare(name, Qt::CaseInsensitive) == 0) {
                d.isDefault = d.isDefault || isDefault;
                d.version   = (version == 2) ? WslVersion::WSL2 : WslVersion::WSL1;
                if (state.contains("running") || state.contains("运行"))
                    d.state = DistroState::Running;
                else if (state.contains("stopped") || state.contains("停止"))
                    d.state = DistroState::Stopped;
                break;
            }
        }
    }
}

bool WslManager::terminateDistribution(const QString &name)
{
    QString out;
    return runProcess("wsl.exe", {"-t", name}, out, 15000);
}

bool WslManager::setDefaultUser(const QString &distroName, const QString &userName)
{
    // 通过发行版自身 exe 设置 (Ubuntu.exe, Debian.exe 等)
    // 若不存在则通过 wsl -d 执行 usermod
    QSettings lxss(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Lxss",
        QSettings::NativeFormat);

    QStringList guids = lxss.childGroups();
    for (const QString &guid : guids) {
        lxss.beginGroup(guid);
        QString n = lxss.value("DistributionName", "").toString();
        lxss.endGroup();
        if (n.compare(distroName, Qt::CaseInsensitive) == 0) {
            // 获取 uid
            QString out;
            QString cmd = QString("id -u %1").arg(userName);
            if (runProcess("wsl.exe", {"-d", distroName, "-u", "root", "--", "bash", "-c", cmd}, out, 10000)) {
                bool ok;
                int uid = out.trimmed().toInt(&ok);
                if (ok) {
                    // 写入注册表
                    QSettings reg(
                        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Lxss\\" + guid,
                        QSettings::NativeFormat);
                    reg.setValue("DefaultUid", uid);
                    return true;
                }
            }
        }
    }
    return false;
}

bool WslManager::runProcess(const QString &program,
                            const QStringList &args,
                            QString &output,
                            int timeoutMs)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);

    proc.start(program, args);
    if (!proc.waitForStarted(3000)) return false;
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        return false;
    }

    QByteArray raw = proc.readAllStandardOutput();
    if (raw.startsWith("\xFF\xFE") || (raw.size() >= 2 && raw[1] == '\0')) {
        int startOffset = raw.startsWith("\xFF\xFE") ? 2 : 0;
        output = QString::fromUtf16(reinterpret_cast<const ushort*>(raw.constData() + startOffset), (raw.size() - startOffset) / 2);
    } else {
        output = QString::fromUtf8(raw);
        if (output.contains(QChar::ReplacementCharacter)) {
            output = QString::fromLocal8Bit(raw);
        }
    }

    // wsl -l -v 在 exit code != 0 时仍有有效输出
    return true;
}

QString WslManager::detectDistroDisplayName(const QString &regName)
{
    // 根据注册表名称匹配常见显示名
    struct { const char *key; const char *display; } table[] = {
        {"ubuntu-24",   "Ubuntu 24.04 LTS"},
        {"ubuntu-22",   "Ubuntu 22.04 LTS"},
        {"ubuntu-20",   "Ubuntu 20.04 LTS"},
        {"ubuntu-18",   "Ubuntu 18.04 LTS"},
        {"ubuntu",      "Ubuntu"},
        {"debian",      "Debian GNU/Linux"},
        {"kali-linux",  "Kali Linux"},
        {"opensuse-leap-15", "openSUSE Leap 15"},
        {"opensuse-tumbleweed", "openSUSE Tumbleweed"},
        {"fedoraremix", "Fedora Remix"},
        {"alpine",      "Alpine Linux"},
        {"arch",        "Arch Linux"},
        {"oracle",      "Oracle Linux"},
        {"amazon",      "Amazon Linux"},
        {nullptr, nullptr}
    };

    QString lower = regName.toLower();
    for (int i = 0; table[i].key; i++) {
        if (lower.contains(table[i].key)) {
            return table[i].display;
        }
    }
    return regName; // 原名兜底
}

QString WslManager::findVhdxPath(const QString &basePath)
{
    if (basePath.isEmpty()) return "";

    // WSL2: <basePath>\ext4.vhdx
    QString candidate = QDir::toNativeSeparators(basePath + "/ext4.vhdx");
    if (QFileInfo::exists(candidate)) return candidate;

    // 有些版本放在 LocalState 子目录
    candidate = QDir::toNativeSeparators(basePath + "/LocalState/ext4.vhdx");
    if (QFileInfo::exists(candidate)) return candidate;

    // WSL1: rootfs 目录（不是 vhdx，返回目录路径用于计算）
    QString rootfs = QDir::toNativeSeparators(basePath + "/rootfs");
    if (QDir(rootfs).exists()) return rootfs;

    return "";
}

qint64 WslManager::getFileSize(const QString &path)
{
    QFileInfo fi(path);
    if (fi.exists() && fi.isFile()) return fi.size();
    if (fi.exists() && fi.isDir()) {
        // WSL1: 递归计算目录大小（粗略）
        qint64 total = 0;
        QDir dir(path);
        QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) { it.next(); total += it.fileInfo().size(); }
        return total;
    }
    return -1;
}

