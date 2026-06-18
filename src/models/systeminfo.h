#pragma once

#include <QString>
#include <QStringList>

struct SystemInfo {
    // Windows 信息
    QString windowsProductName;   // e.g. "Windows 11 Pro"
    QString windowsBuild;         // e.g. "22631"
    QString windowsVersion;       // e.g. "23H2"
    QString windowsEdition;       // e.g. "Pro"
    QString windowsUBR;           // Update Build Revision
    QString architecture;         // e.g. "x64"
    bool    isWindows11;

    // WSL 信息
    bool    wslInstalled;
    QString wslVersion;           // WSL 本体版本
    QString wslKernelVersion;     // Linux 内核版本
    QString wslgVersion;          // WSLg 版本
    QString defaultWslVersion;    // 默认新建使用 WSL1 or WSL2
    bool    wsl2Supported;

    QString windowsDisplayString() const {
        QString s = windowsProductName;
        if (!windowsVersion.isEmpty()) s += " " + windowsVersion;
        if (!windowsBuild.isEmpty())   s += " (Build " + windowsBuild;
        if (!windowsUBR.isEmpty())     s += "." + windowsUBR;
        if (!windowsBuild.isEmpty())   s += ")";
        return s;
    }
};

