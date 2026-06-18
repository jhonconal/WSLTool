#pragma once
#include <QWidget>
#include <QList>
#include "../models/systeminfo.h"
#include "../models/wsldistribution.h"
#include "../models/diskinfo.h"

class DashboardPage : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardPage(QWidget *parent = nullptr);
    void setData(const SystemInfo &sys,
                 const QList<WslDistribution> &distros,
                 const QList<DiskInfo> &disks);
    void updateTheme(bool isDark);

private:
    void setupUi();
    void buildSysCards();
    void buildDisksSection();
    void buildStatsSection();

    SystemInfo              m_sys;
    QList<WslDistribution>  m_distros;
    QList<DiskInfo>         m_disks;

    QWidget *m_sysCardArea;
    QWidget *m_diskArea;
    QWidget *m_statsArea;
};

