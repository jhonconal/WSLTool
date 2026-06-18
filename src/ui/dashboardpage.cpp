#include "dashboardpage.h"
#include "widgets/infocard.h"
#include "widgets/diskusagebar.h"
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>

DashboardPage::DashboardPage(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void DashboardPage::setupUi()
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->setContentsMargins(28, 24, 28, 24);
    vl->setSpacing(24);

    // 页面标题
    QLabel *pageTitle = new QLabel("系统概览");
    pageTitle->setObjectName("pageTitle");
    vl->addWidget(pageTitle);

    QLabel *pageSub = new QLabel("当前 Windows 与 WSL 环境信息");
    pageSub->setObjectName("pageSub");
    vl->addWidget(pageSub);

    // 系统信息卡片区
    QLabel *sec1 = new QLabel("Windows 系统信息");
    sec1->setObjectName("sectionHeader");
    vl->addWidget(sec1);

    m_sysCardArea = new QWidget;
    m_sysCardArea->setLayout(new QHBoxLayout);
    m_sysCardArea->layout()->setContentsMargins(0,0,0,0);
    static_cast<QHBoxLayout*>(m_sysCardArea->layout())->setSpacing(12);
    vl->addWidget(m_sysCardArea);

    // 磁盘区
    QLabel *sec2 = new QLabel("磁盘使用情况");
    sec2->setObjectName("sectionHeader");
    vl->addWidget(sec2);

    m_diskArea = new QWidget;
    m_diskArea->setLayout(new QHBoxLayout);
    m_diskArea->layout()->setContentsMargins(0,0,0,0);
    static_cast<QHBoxLayout*>(m_diskArea->layout())->setSpacing(12);
    vl->addWidget(m_diskArea);

    // 统计区
    QLabel *sec3 = new QLabel("WSL 统计");
    sec3->setObjectName("sectionHeader");
    vl->addWidget(sec3);

    m_statsArea = new QWidget;
    m_statsArea->setLayout(new QHBoxLayout);
    m_statsArea->layout()->setContentsMargins(0,0,0,0);
    static_cast<QHBoxLayout*>(m_statsArea->layout())->setSpacing(12);
    vl->addWidget(m_statsArea);

    vl->addStretch();
}

void DashboardPage::setData(const SystemInfo &sys,
                             const QList<WslDistribution> &distros,
                             const QList<DiskInfo> &disks)
{
    m_sys    = sys;
    m_distros = distros;
    m_disks  = disks;
    buildSysCards();
    buildDisksSection();
    buildStatsSection();
}

void DashboardPage::buildSysCards()
{
    QHBoxLayout *hl = static_cast<QHBoxLayout*>(m_sysCardArea->layout());
    // 清除旧的
    QLayoutItem *item;
    while ((item = hl->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    auto addCard = [&](const QString &title, const QString &val,
                       const QString &sub, const QString &icon,
                       const QString &color = "#00d4aa") {
        InfoCard *c = new InfoCard(title);
        c->setValue(val);
        c->setSubValue(sub);
        c->setIcon(icon);
        c->setAccentColor(color);
        hl->addWidget(c, 1);
    };

    addCard("操作系统",
            m_sys.windowsProductName,
            m_sys.windowsVersion + "  Build " + m_sys.windowsBuild + "." + m_sys.windowsUBR,
            ":/icons/card_windows.svg",
            "#6366f1");

    addCard("WSL 状态",
            m_sys.wslInstalled ? "已安装" : "未安装",
            m_sys.wslVersion.isEmpty() ? "" : "版本 " + m_sys.wslVersion,
            m_sys.wslInstalled ? ":/icons/card_wsl_ok.svg" : ":/icons/card_wsl_fail.svg",
            m_sys.wslInstalled ? "#00d4aa" : "#ff4d6d");

    addCard("内核版本",
            m_sys.wslKernelVersion.isEmpty() ? "—" : m_sys.wslKernelVersion,
            m_sys.wslgVersion.isEmpty() ? "" : "WSLg " + m_sys.wslgVersion,
            ":/icons/card_kernel.svg",
            "#f59e0b");

    addCard("默认版本",
            m_sys.defaultWslVersion.isEmpty() ? "—" : "WSL " + m_sys.defaultWslVersion,
            m_sys.wsl2Supported ? "支持 WSL2" : "不支持 WSL2",
            ":/icons/card_version.svg",
            "#6366f1");
}

void DashboardPage::buildDisksSection()
{
    QHBoxLayout *hl = static_cast<QHBoxLayout*>(m_diskArea->layout());
    QLayoutItem *item;
    while ((item = hl->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (const DiskInfo &d : m_disks) {
        DiskUsageBar *bar = new DiskUsageBar;
        bar->setDiskInfo(d.letter, d.label, d.usedBytes, d.totalBytes, d.formattedFree());
        hl->addWidget(bar, 1);
    }
    if (m_disks.isEmpty()) {
        QLabel *empty = new QLabel("无固定磁盘信息");
        empty->setObjectName("emptyLabel");
        hl->addWidget(empty);
    }
    hl->addStretch();
}

void DashboardPage::buildStatsSection()
{
    QHBoxLayout *hl = static_cast<QHBoxLayout*>(m_statsArea->layout());
    QLayoutItem *item;
    while ((item = hl->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    int total    = m_distros.size();
    int onCDrive = 0;
    int wsl2cnt  = 0;
    int running  = 0;
    for (const auto &d : m_distros) {
        if (d.isOnCDrive) onCDrive++;
        if (d.version == WslVersion::WSL2) wsl2cnt++;
        if (d.state == DistroState::Running) running++;
    }

    auto addCard = [&](const QString &title, const QString &val,
                       const QString &sub, const QString &icon,
                       const QString &color = "#00d4aa") {
        InfoCard *c = new InfoCard(title);
        c->setValue(val);
        c->setSubValue(sub);
        c->setIcon(icon);
        c->setAccentColor(color);
        hl->addWidget(c, 1);
    };

    addCard("发行版数量",  QString::number(total),    "已安装",   ":/icons/tux.svg");
    addCard("C盘发行版",   QString::number(onCDrive), "建议迁移", ":/icons/card_warning.svg",  "#ff4d6d");
    addCard("WSL2 数量",   QString::number(wsl2cnt),  "推荐版本", ":/icons/card_play.svg",  "#6366f1");
    addCard("运行中",      QString::number(running),  "当前活跃", ":/icons/card_wsl_ok.svg",  "#22c55e");
    hl->addStretch();
}

void DashboardPage::updateTheme(bool isDark)
{
    QHBoxLayout *hl = static_cast<QHBoxLayout*>(m_diskArea->layout());
    for (int i = 0; i < hl->count(); i++) {
        QLayoutItem *item = hl->itemAt(i);
        if (item && item->widget()) {
            DiskUsageBar *bar = qobject_cast<DiskUsageBar*>(item->widget());
            if (bar) {
                bar->updateTheme(isDark);
            }
        }
    }
}

