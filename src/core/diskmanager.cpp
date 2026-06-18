#include "diskmanager.h"
#include <windows.h>

DiskManager::DiskManager(QObject *parent) : QObject(parent) {}

QList<DiskInfo> DiskManager::enumerateFixedDisks()
{
    QList<DiskInfo> list;

    DWORD driveMask = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (!(driveMask & (1 << i))) continue;

        QString letter = QString(QChar('A' + i));
        QString root   = letter + ":\\";

        // 只枚举固定磁盘
        UINT driveType = GetDriveTypeW(reinterpret_cast<const wchar_t*>(root.utf16()));
        if (driveType != DRIVE_FIXED) continue;

        DiskInfo info = getDiskInfo(letter);
        if (info.totalBytes > 0) {
            list.append(info);
        }
    }

    return list;
}

DiskInfo DiskManager::getDiskInfo(const QString &letter)
{
    DiskInfo info;
    info.letter = letter.toUpper();

    QString root = letter + ":\\";

    // 获取卷标和文件系统
    wchar_t volName[MAX_PATH + 1] = {0};
    wchar_t fsName[MAX_PATH + 1]  = {0};
    DWORD serialNum = 0, maxCompLen = 0, fsFlags = 0;
    if (GetVolumeInformationW(
            reinterpret_cast<const wchar_t*>(root.utf16()),
            volName, MAX_PATH,
            &serialNum, &maxCompLen, &fsFlags,
            fsName, MAX_PATH)) {
        info.label      = QString::fromWCharArray(volName);
        info.fileSystem = QString::fromWCharArray(fsName);
    }

    // 获取磁盘空间
    ULARGE_INTEGER freeBytesAvail, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceExW(
            reinterpret_cast<const wchar_t*>(root.utf16()),
            &freeBytesAvail, &totalBytes, &totalFreeBytes)) {
        info.totalBytes = static_cast<qint64>(totalBytes.QuadPart);
        info.freeBytes  = static_cast<qint64>(freeBytesAvail.QuadPart);
        info.usedBytes  = info.totalBytes - static_cast<qint64>(totalFreeBytes.QuadPart);
    }

    return info;
}

bool DiskManager::hasEnoughSpace(const QString &diskLetter, qint64 requiredBytes)
{
    DiskInfo info = getDiskInfo(diskLetter);
    return info.hasEnoughSpace(requiredBytes);
}

