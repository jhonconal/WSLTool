#include "onlinedistropage.h"
#include "widgets/onlinedistrocard.h"
#include "installdialog.h"
#include "installprogressdialog.h"
#include "../core/wslmanager.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStyle>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

OnlineDistroPage::OnlineDistroPage(QWidget *parent) : QWidget(parent)
{
    m_watcher = new QFutureWatcher<QList<OnlineDistribution>>(this);
    connect(m_watcher, &QFutureWatcher<QList<OnlineDistribution>>::finished, this, &OnlineDistroPage::onFetchFinished);

    setupUi();
}

OnlineDistroPage::~OnlineDistroPage()
{
}

void OnlineDistroPage::setupUi()
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->setContentsMargins(28, 24, 28, 24);
    vl->setSpacing(16);

    // 页面标题
    QLabel *title = new QLabel("在线发行版");
    title->setObjectName("pageTitle");
    vl->addWidget(title);

    QLabel *sub = new QLabel("浏览可用的 Linux 发行版，支持一键下载安装，并支持直接部署到指定非系统盘");
    sub->setObjectName("pageSub");
    vl->addWidget(sub);

    // 状态加载指示器
    m_loadingLabel = new QLabel("正在获取在线发行版列表，请稍候...");
    m_loadingLabel->setObjectName("emptyLabel"); // 使用空状态的样式
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    vl->addWidget(m_loadingLabel, 1);

    // 滚动区域
    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setObjectName("pageScroll");
    m_scrollArea->setVisible(false);

    QWidget *scrollContent = new QWidget;
    scrollContent->setObjectName("pageScrollContent");
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(10);

    m_listArea = scrollContent;
    m_scrollArea->setWidget(scrollContent);
    vl->addWidget(m_scrollArea, 1);

    m_emptyLabel = new QLabel("获取在线发行版列表失败\n\n请检查您的网络连接并重试");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setObjectName("emptyLabel");
    m_emptyLabel->setVisible(false);
    vl->addWidget(m_emptyLabel);
}

void OnlineDistroPage::loadDataAsync(const QList<WslDistribution> &installedDistros,
                                    const QList<DiskInfo> &disks)
{
    m_installedDistros = installedDistros;
    m_disks = disks;

    if (m_watcher->isRunning()) {
        return;
    }

    m_loadingLabel->setVisible(true);
    m_scrollArea->setVisible(false);
    m_emptyLabel->setVisible(false);

    QFuture<QList<OnlineDistribution>> future = QtConcurrent::run([](const QList<WslDistribution> &installed) {
        WslManager wm;
        return wm.enumerateOnlineDistributions(installed);
    }, m_installedDistros);

    m_watcher->setFuture(future);
}

void OnlineDistroPage::onFetchFinished()
{
    m_onlineDistros = m_watcher->result();
    m_loadingLabel->setVisible(false);

    if (m_onlineDistros.isEmpty()) {
        m_emptyLabel->setVisible(true);
        m_scrollArea->setVisible(false);
    } else {
        m_emptyLabel->setVisible(false);
        m_scrollArea->setVisible(true);
        populateList();
    }
}

void OnlineDistroPage::populateList()
{
    QVBoxLayout *vl = static_cast<QVBoxLayout*>(m_listArea->layout());

    // 清除旧卡片
    QLayoutItem *item;
    while ((item = vl->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (const OnlineDistribution &distro : m_onlineDistros) {
        OnlineDistroCard *card = new OnlineDistroCard(distro, m_listArea);
        connect(card, &OnlineDistroCard::installRequested, this, &OnlineDistroPage::onInstallRequested);
        vl->addWidget(card);
    }
    vl->addStretch();
}

void OnlineDistroPage::onInstallRequested(const OnlineDistribution &distro)
{
    InstallDialog dlg(distro, m_disks, this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    InstallConfig config;
    config.distroName = distro.name;
    config.friendlyName = distro.friendlyName;
    config.isDefaultPath = dlg.isDefaultInstall();
    config.targetDirectory = dlg.targetPath();
    config.createAccount = dlg.createAccount();
    config.username = dlg.username();
    config.password = dlg.password();

    InstallProgressDialog *prog = new InstallProgressDialog(config, this);
    prog->setAttribute(Qt::WA_DeleteOnClose);
    if (prog->exec() == QDialog::Accepted) {
        emit refreshNeeded();
    }
}
