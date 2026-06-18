#pragma once

#include <QString>
#include <QList>

enum class WslVersion {
    WSL1 = 1,
    WSL2 = 2,
    Unknown = 0
};

enum class DistroState {
    Running,
    Stopped,
    Installing,
    Unknown
};

struct WslDistribution {
    QString name;           // 发行版名称，e.g. "Ubuntu-22.04"
    QString displayName;    // 显示名称
    QString basePath;       // 安装目录，e.g. "C:\Users\...\AppData\Local\Packages\..."
    QString vhdxPath;       // ext4.vhdx 完整路径
    QString diskLetter;     // 所在盘符，e.g. "C"
    qint64  sizeBytes;      // vhdx 文件大小（字节）
    WslVersion version;     // WSL1 或 WSL2
    DistroState state;      // 运行状态
    bool isDefault;         // 是否为默认发行版
    bool isOnCDrive;        // 是否在C盘
    QString registryGuid;   // 注册表 GUID

    // 格式化显示
    QString formattedSize() const {
        if (sizeBytes < 0) return "未知";
        double gb = sizeBytes / (1024.0 * 1024.0 * 1024.0);
        if (gb >= 1.0)
            return QString::number(gb, 'f', 1) + " GB";
        double mb = sizeBytes / (1024.0 * 1024.0);
        return QString::number(mb, 'f', 0) + " MB";
    }

    QString versionString() const {
        switch (version) {
        case WslVersion::WSL1: return "WSL 1";
        case WslVersion::WSL2: return "WSL 2";
        default: return "未知";
        }
    }

    QString stateString() const {
        switch (state) {
        case DistroState::Running:    return "运行中";
        case DistroState::Stopped:    return "已停止";
        case DistroState::Installing: return "安装中";
        default: return "未知";
        }
    }
};

