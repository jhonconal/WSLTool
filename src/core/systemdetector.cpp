#include "systemdetector.h"
#include <QProcess>
#include <QSettings>
#include <QSysInfo>
#include <QRegularExpression>
#include <windows.h>

SystemDetector::SystemDetector(QObject *parent) : QObject(parent) {}

SystemInfo SystemDetector::detect()
{
    SystemInfo info = detectWindows();
    detectWsl(info);
    return info;
}

SystemInfo SystemDetector::detectWindows()
{
    SystemInfo info;

    // 通过注册表读取 Windows 版本（兼容 Win7/8/10/11）
    QSettings reg(
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        QSettings::NativeFormat);

    info.windowsProductName = reg.value("ProductName", "Windows").toString();
    info.windowsBuild       = reg.value("CurrentBuild", "").toString();
    info.windowsUBR         = reg.value("UBR", "").toString();

    // Win10 1903+ 有 DisplayVersion (e.g. "23H2")
    // 更早版本有 ReleaseId (e.g. "1909")
    QString displayVer = reg.value("DisplayVersion", "").toString();
    if (displayVer.isEmpty())
        displayVer = reg.value("ReleaseId", "").toString();
    info.windowsVersion = displayVer;

    // 判断 Windows 11: Build >= 22000
    bool ok = false;
    int buildNum = info.windowsBuild.toInt(&ok);
    info.isWindows11 = (ok && buildNum >= 22000);

    // 若 ProductName 仍是 "Windows 10" 但 Build>=22000，修正为 Windows 11
    if (info.isWindows11 && info.windowsProductName.contains("Windows 10")) {
        info.windowsProductName.replace("Windows 10", "Windows 11");
    }

    // 架构
    info.architecture = QSysInfo::currentCpuArchitecture();

    return info;
}

void SystemDetector::detectWsl(SystemInfo &info)
{
    info.wslInstalled      = false;
    info.wsl2Supported     = false;
    info.wslVersion        = "";
    info.wslKernelVersion  = "";
    info.wslgVersion       = "";
    info.defaultWslVersion = "";

    // WSL2 需要 Build >= 18362 (Win10 1903)
    bool ok = false;
    int buildNum = info.windowsBuild.toInt(&ok);
    if (ok && buildNum >= 18362) {
        info.wsl2Supported = true;
    }

    // 尝试 wsl --version (较新版本支持)
    QString out;
    if (runProcess("wsl.exe", {"--version"}, out, 6000)) {
        info.wslInstalled = true;
        // 解析: "WSL 版本: 2.3.26.0"
        QRegularExpression reWsl(R"(WSL[^\d]*(\d[\d\.]+))");
        QRegularExpression reKernel(R"(内核版本[:：]\s*([\d\.\-]+)|Kernel version[:：]\s*([\d\.\-]+))");
        QRegularExpression reWslg(R"(WSLg[^\d]*([\d\.]+))");

        auto m = reWsl.match(out);
        if (m.hasMatch()) info.wslVersion = m.captured(1);

        m = reKernel.match(out);
        if (m.hasMatch()) info.wslKernelVersion = m.captured(1).isEmpty() ? m.captured(2) : m.captured(1);

        m = reWslg.match(out);
        if (m.hasMatch()) info.wslgVersion = m.captured(1);
    } else {
        // 旧版本：尝试 wslconfig /list
        QString out2;
        if (runProcess("wslconfig.exe", {"/list"}, out2, 6000)) {
            info.wslInstalled = true;
        } else {
            // 兜底：检查注册表是否存在 WSL key
            QSettings lxss("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Lxss",
                            QSettings::NativeFormat);
            QStringList keys = lxss.childGroups();
            info.wslInstalled = !keys.isEmpty();
        }
    }

    // 默认 WSL 版本
    QString statusOut;
    if (runProcess("wsl.exe", {"--status"}, statusOut, 5000)) {
        QRegularExpression re(R"(默认版本[:：]\s*(\d)|Default Version[:：]\s*(\d))");
        auto m = re.match(statusOut);
        if (m.hasMatch()) {
            info.defaultWslVersion = m.captured(1).isEmpty() ? m.captured(2) : m.captured(1);
        }
    }
}

bool SystemDetector::runProcess(const QString &program,
                                const QStringList &args,
                                QString &output,
                                int timeoutMs)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    // 设置 UTF-8 环境
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LANG", "en_US.UTF-8");
    proc.setProcessEnvironment(env);

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

    return proc.exitCode() == 0;
}

