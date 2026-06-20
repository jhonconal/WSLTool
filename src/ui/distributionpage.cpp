#include "distributionpage.h"
#include "widgets/distrocard.h"
#include "migrationdialog.h"
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QFrame>
#include <QProcess>
#include <QMessageBox>
#include <QApplication>

DistributionPage::DistributionPage(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void DistributionPage::setupUi()
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->setContentsMargins(28, 24, 28, 24);
    vl->setSpacing(16);

    // 页面标题
    QLabel *title = new QLabel("WSL 发行版管理");
    title->setObjectName("pageTitle");
    vl->addWidget(title);

    QLabel *sub = new QLabel("管理已安装的 WSL Linux 发行版，支持一键迁移到其他磁盘");
    sub->setObjectName("pageSub");
    vl->addWidget(sub);

    // 免责提示横幅
    QWidget *banner = new QWidget;
    banner->setObjectName("warnBanner");
    QHBoxLayout *bhl = new QHBoxLayout(banner);
    bhl->setContentsMargins(16, 12, 16, 12);
    QLabel *warnIcon = new QLabel();
    warnIcon->setPixmap(QPixmap(":/icons/card_warning.svg").scaled(16, 16));
    QLabel *warn = new QLabel(
        "<b>重要提示</b>：迁移操作存在数据丢失风险！"
        "迁移前请务必做好数据备份。本工具仅作辅助，"
        "因操作不当导致的数据损失概不负责。");
    warn->setObjectName("warnText");
    warn->setWordWrap(true);
    bhl->addWidget(warnIcon);
    bhl->addWidget(warn, 1);
    vl->addWidget(banner);

    // 滚动区域
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setObjectName("pageScroll");

    QWidget *scrollContent = new QWidget;
    scrollContent->setObjectName("pageScrollContent");
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(10);

    m_listArea = scrollContent;
    scroll->setWidget(scrollContent);
    vl->addWidget(scroll, 1);

    m_emptyLabel = new QLabel("未检测到已安装的 WSL 发行版\n\n"
                               "请先在 Microsoft Store 或通过命令行安装 WSL 发行版");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setObjectName("emptyLabel");
    m_emptyLabel->setVisible(false);
}

void DistributionPage::setData(const QList<WslDistribution> &distros,
                                const QList<DiskInfo> &disks)
{
    m_distros = distros;
    m_disks   = disks;
    populateList();
}

void DistributionPage::populateList()
{
    QVBoxLayout *vl = static_cast<QVBoxLayout*>(m_listArea->layout());

    // 清除旧卡片
    QLayoutItem *item;
    while ((item = vl->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    if (m_distros.isEmpty()) {
        vl->addWidget(m_emptyLabel);
        m_emptyLabel->setVisible(true);
        return;
    }
    m_emptyLabel->setVisible(false);

    // C盘先，其他后
    QList<WslDistribution> sorted;
    for (const auto &d : m_distros) if (d.isOnCDrive)  sorted.append(d);
    for (const auto &d : m_distros) if (!d.isOnCDrive) sorted.append(d);

    for (const WslDistribution &distro : sorted) {
        DistroCard *card = new DistroCard(distro, m_listArea);
        connect(card, &DistroCard::migrateRequested,
                this, &DistributionPage::onMigrateRequested);
        connect(card, &DistroCard::deleteRequested,
                this, &DistributionPage::onDeleteRequested);
        connect(card, &DistroCard::refreshNeeded,
                this, &DistributionPage::refreshNeeded);
        vl->addWidget(card);
    }
    vl->addStretch();
}

void DistributionPage::onMigrateRequested(const WslDistribution &distro)
{
    MigrationDialog dlg(distro, m_disks, this);
    dlg.exec();

    // 迁移后刷新列表（等待外部 refresh 信号或用户手动刷新）
}

void DistributionPage::onDeleteRequested(const WslDistribution &distro)
{
    QString confirmMsg = QString(
        "警告：此操作将<b>彻底删除</b>发行版 <b>%1</b> 及其所有本地虚拟磁盘和文件数据！<br><br>"
        "该操作是<b>不可逆</b>的，且将丢失所有该发行版内的个人文件和配置！<br>"
        "请确认是否继续？")
        .arg(distro.displayName.isEmpty() ? distro.name : distro.displayName);

    QMessageBox mb(QMessageBox::Warning, "确认删除", confirmMsg,
                   QMessageBox::Yes | QMessageBox::No, this);
    mb.button(QMessageBox::Yes)->setText("确认彻底删除");
    mb.button(QMessageBox::No)->setText("取消");

    if (mb.exec() != QMessageBox::Yes) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QProcess proc;
    proc.start("wsl.exe", {"--unregister", distro.name});
    if (proc.waitForStarted(3000) && proc.waitForFinished(30000)) {
        QApplication::restoreOverrideCursor();
        if (proc.exitCode() == 0) {
            QMessageBox::information(this, "删除成功", QString("发行版 <b>%1</b> 已成功注销并清理！").arg(distro.name));
            emit refreshNeeded();
        } else {
            QString err = QString::fromLocal8Bit(proc.readAllStandardOutput());
            if (err.isEmpty()) err = QString::fromLocal8Bit(proc.readAllStandardError());
            QMessageBox::critical(this, "删除失败", QString("删除 <b>%1</b> 失败：<br>%2").arg(distro.name, err));
        }
    } else {
        QApplication::restoreOverrideCursor();
        proc.kill();
        QMessageBox::critical(this, "删除失败", "删除操作已超时或无法启动进程。");
    }
}

