#pragma once

#include <QString>

struct DiskInfo {
    QString letter;         // 盘符，e.g. "C"
    QString label;          // 卷标
    QString fileSystem;     // 文件系统类型
    qint64  totalBytes;     // 总容量
    qint64  freeBytes;      // 可用容量
    qint64  usedBytes;      // 已用容量

    double usagePercent() const {
        if (totalBytes <= 0) return 0.0;
        return (double)usedBytes / totalBytes * 100.0;
    }

    double freePercent() const {
        if (totalBytes <= 0) return 0.0;
        return (double)freeBytes / totalBytes * 100.0;
    }

    QString formattedTotal() const { return formatBytes(totalBytes); }
    QString formattedFree() const  { return formatBytes(freeBytes); }
    QString formattedUsed() const  { return formatBytes(usedBytes); }

    static QString formatBytes(qint64 bytes) {
        if (bytes < 0) return "未知";
        if (bytes >= (qint64)1024*1024*1024*1024)
            return QString::number(bytes / (1024.0*1024.0*1024.0*1024.0), 'f', 2) + " TB";
        if (bytes >= 1024*1024*1024)
            return QString::number(bytes / (1024.0*1024.0*1024.0), 'f', 1) + " GB";
        if (bytes >= 1024*1024)
            return QString::number(bytes / (1024.0*1024.0), 'f', 0) + " MB";
        return QString::number(bytes / 1024.0, 'f', 0) + " KB";
    }

    bool hasEnoughSpace(qint64 requiredBytes) const {
        return freeBytes >= (qint64)(requiredBytes * 1.25); // 需要 125% 空间
    }
};

