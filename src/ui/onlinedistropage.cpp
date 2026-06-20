#include "onlinedistropage.h"
#include "widgets/onlinedistrocard.h"
#include "installdialog.h"
#include "migrationprogressdialog.h"
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
    if (m_installProcess) {
        m_installProcess->kill();
        m_installProcess->deleteLater();
    }
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
    if (m_installProcess && m_installProcess->state() != QProcess::NotRunning) {
        QMessageBox::warning(this, "正在安装", "当前已有正在进行的安装任务，请等待完成。");
        return;
    }

    InstallDialog dlg(distro, m_disks, this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    startInstall(distro, dlg.isDefaultInstall(), dlg.targetPath());
}

void OnlineDistroPage::startInstall(const OnlineDistribution &distro, bool isDefault, const QString &targetPath)
{
    m_installingDistro = distro;
    m_installTargetPath = isDefault ? "" : targetPath;

    // 提示用户
    QMessageBox::information(this, "开始安装",
        "正在启动 WSL 安装程序，将在新打开的<b>控制台终端</b>窗口中进行下载和设置。<br><br>"
        "<b>重要步骤：</b><br>"
        "1. 请在弹出的命令行窗口中耐性等待其下载完成（可能需要数分钟）。<br>"
        "2. 下载完成后，<b>必须</b>按照命令行提示<b>创建您的 Linux 用户名和密码</b>。<br>"
        "3. 配置完成后，可以输入 <code>exit</code> 退出，或直接关闭该终端窗口。<br>"
        "4. 终端关闭后，本工具将继续完成后续部署（如果是指定路径，则会自动迁移）。",
        QMessageBox::Ok);

    m_installProcess = new QProcess(this);
    connect(m_installProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &OnlineDistroPage::onInstallProcessFinished);

    // 在新窗口中执行 wsl --install 并等待结束
    // cmd.exe /c start /wait wsl.exe --install <name>
    m_installProcess->start("cmd.exe", {"/c", "start", "/wait", "wsl.exe", "--install", distro.name});
}

void OnlineDistroPage::onInstallProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);

    m_installProcess->deleteLater();
    m_installProcess = nullptr;

    // 检查发行版是否已注册成功
    WslManager wm;
    QList<WslDistribution> currentList = wm.enumerateDistributions();
    WslDistribution targetDistro;
    bool found = false;

    for (const auto &d : currentList) {
        if (d.name.compare(m_installingDistro.name, Qt::CaseInsensitive) == 0) {
            targetDistro = d;
            found = true;
            break;
        }
    }

    if (!found) {
        QMessageBox::warning(this, "安装失败",
            QString("未检测到 <b>%1</b> 的注册信息，安装可能已被取消或发生错误。")
            .arg(m_installingDistro.friendlyName));
        emit refreshNeeded();
        return;
    }

    // 如果指定了自定义路径，启动迁移
    if (!m_installTargetPath.isEmpty()) {
        MigrationConfig config;
        config.distro = targetDistro;
        config.targetDirectory = m_installTargetPath;
        config.configureAccount = false; // 不需要重新配置账户，我们稍后通过保留 originalUid 解决，或者由于用户已经设置好，在 registry 里面能读到 default user

        MigrationProgressDialog *prog = new MigrationProgressDialog(config, this);
        prog->setAttribute(Qt::WA_DeleteOnClose);
        prog->exec();
    } else {
        QMessageBox::information(this, "安装成功",
            QString("发行版 <b>%1</b> 已成功下载并安装在默认路径！")
            .arg(m_installingDistro.friendlyName));
    }

    emit refreshNeeded();
}
