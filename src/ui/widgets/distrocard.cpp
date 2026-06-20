#include "distrocard.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QVariant>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QTimer>
#include "../../mainwindow.h"

DistroCard::DistroCard(const WslDistribution &distro, QWidget *parent)
    : QWidget(parent), m_distro(distro)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumHeight(100);
    setMaximumHeight(120);
    setCursor(Qt::ArrowCursor);
    
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &DistroCard::showContextMenu);
    
    setupUi();
}

void DistroCard::setupUi()
{
    QHBoxLayout *hl = new QHBoxLayout(this);
    hl->setContentsMargins(20, 16, 20, 16);
    hl->setSpacing(16);

    // 发行版图标 (SVG)
    struct { const char *key; const char *path; } iconMap[] = {
        {"ubuntu",   ":/icons/ubuntu.svg"},
        {"debian",   ":/icons/debian.svg"},
        {"fedora",   ":/icons/fedora.svg"},
        {"arch",     ":/icons/archlinux.svg"},
        {"opensuse", ":/icons/opensuse.svg"},
        {"alpine",   ":/icons/alpine.svg"},
        {"kali",     ":/icons/kali.svg"},
        {"oracle",   ":/icons/oracle.svg"},
        {"amazon",   ":/icons/amazon.svg"},
        {nullptr, nullptr}
    };
    QString iconPath = ":/icons/tux.svg";
    QString lower = m_distro.name.toLower();
    for (int i = 0; iconMap[i].key; i++) {
        if (lower.contains(iconMap[i].key)) { iconPath = iconMap[i].path; break; }
    }

    m_iconLabel = new QLabel;
    m_iconLabel->setObjectName("distroIcon");
    m_iconLabel->setFixedSize(48, 48);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    QPixmap iconPix(iconPath);
    if (!iconPix.isNull()) {
        m_iconLabel->setPixmap(iconPix.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    // 中部信息
    QVBoxLayout *vl = new QVBoxLayout;
    vl->setSpacing(4);
    vl->setContentsMargins(0, 0, 0, 0);

    // 第一行：名称 + Default 标记
    QHBoxLayout *row1 = new QHBoxLayout;
    row1->setSpacing(8);
    m_nameLabel = new QLabel(m_distro.displayName.isEmpty() ? m_distro.name : m_distro.displayName);
    m_nameLabel->setObjectName("distroName");

    if (m_distro.isDefault) {
        QLabel *defBadge = new QLabel("默认");
        defBadge->setObjectName("distroDefaultBadge");
        row1->addWidget(m_nameLabel);
        row1->addWidget(defBadge);
    } else {
        row1->addWidget(m_nameLabel);
    }
    row1->addStretch();

    // 版本 badge
    m_versionBadge = new QLabel(m_distro.versionString());
    m_versionBadge->setObjectName("distroVersionBadge");
    row1->addWidget(m_versionBadge);

    // 状态 badge
    m_stateBadge = new QLabel(m_distro.stateString());
    m_stateBadge->setObjectName("distroStateBadge");
    m_stateBadge->setProperty("state", m_distro.state == DistroState::Running ? "running" : "stopped");
    row1->addWidget(m_stateBadge);

    // 第二行：路径 + 大小
    QHBoxLayout *row2 = new QHBoxLayout;
    row2->setSpacing(12);
    m_pathLabel = new QLabel("路径: " + (m_distro.basePath.isEmpty() ? "未知路径" : m_distro.basePath));
    m_pathLabel->setObjectName("distroPath");
    m_pathLabel->setWordWrap(false);

    m_sizeLabel = new QLabel("大小: " + m_distro.formattedSize());
    m_sizeLabel->setObjectName("distroSize");
    row2->addWidget(m_pathLabel, 1);
    row2->addWidget(m_sizeLabel);

    // 盘符 badge
    m_diskBadge = new QLabel(m_distro.diskLetter.isEmpty() ? "?" : m_distro.diskLetter + "盘");
    m_diskBadge->setObjectName("distroDiskBadge");
    m_diskBadge->setProperty("isOnCDrive", m_distro.isOnCDrive ? "true" : "false");
    row2->addWidget(m_diskBadge);

    vl->addLayout(row1);
    vl->addLayout(row2);

    // 启动/停止按钮
    bool isRunning = (m_distro.state == DistroState::Running);
    m_launchBtn = new QPushButton(isRunning ? "停止" : "启动");
    m_launchBtn->setObjectName("distroLaunchBtn");
    m_launchBtn->setFixedSize(90, 36);
    m_launchBtn->setCursor(Qt::PointingHandCursor);
    
    // 动态生成白色图标（启动为三角形，停止为关闭叉号）
    QString launchIconPath = isRunning ? ":/icons/window_close.svg" : ":/icons/card_play.svg";
    m_launchBtn->setIcon(MainWindow::loadThemeIcon(launchIconPath, "#ffffff"));
    m_launchBtn->setIconSize(QSize(12, 12));
    
    connect(m_launchBtn, &QPushButton::clicked, this, [this, isRunning]() {
        if (isRunning) {
            QProcess proc;
            proc.start("wsl.exe", {"--terminate", m_distro.name});
            if (proc.waitForStarted(3000) && proc.waitForFinished(10000)) {
                emit refreshNeeded();
            }
        } else {
            QProcess::startDetached("cmd.exe", {"/c", "start", "wsl.exe", "-d", m_distro.name});
            QTimer::singleShot(1000, this, [this]() {
                emit refreshNeeded();
            });
        }
    });

    // 主目录按钮
    m_homeBtn = new QPushButton("主目录");
    m_homeBtn->setObjectName("distroHomeBtn");
    m_homeBtn->setFixedSize(90, 36);
    m_homeBtn->setCursor(Qt::PointingHandCursor);
    m_homeBtn->setEnabled(m_distro.state == DistroState::Running);
    connect(m_homeBtn, &QPushButton::clicked, this, [this]() {
        QString path = QString("\\\\wsl.localhost\\%1\\home").arg(m_distro.name);
        QDir homeDir(path);
        if (homeDir.exists()) {
            QStringList subDirs = homeDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            if (!subDirs.isEmpty()) {
                path = path + "\\" + subDirs.first();
            }
        } else {
            path = QString("\\\\wsl$\\%1\\home").arg(m_distro.name);
            homeDir.setPath(path);
            if (homeDir.exists()) {
                QStringList subDirs = homeDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                if (!subDirs.isEmpty()) {
                    path = path + "\\" + subDirs.first();
                }
            } else {
                path = QString("\\\\wsl.localhost\\%1").arg(m_distro.name);
            }
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::toNativeSeparators(path)));
    });

    // 迁移按钮（仅 C 盘显示）
    m_migrateBtn = new QPushButton("迁移");
    m_migrateBtn->setObjectName("distroMigrateBtn");
    m_migrateBtn->setFixedSize(90, 36);
    m_migrateBtn->setCursor(Qt::PointingHandCursor);
    m_migrateBtn->setVisible(m_distro.isOnCDrive);
    connect(m_migrateBtn, &QPushButton::clicked, this, [this]() {
        emit migrateRequested(m_distro);
    });

    hl->addWidget(m_iconLabel);
    hl->addLayout(vl, 1);
    hl->addWidget(m_launchBtn);
    hl->addWidget(m_homeBtn);
    hl->addWidget(m_migrateBtn);
}

void DistroCard::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void DistroCard::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

void DistroCard::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    if (m_hovered) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        // 左边发光条
        QPainterPath accent;
        accent.addRoundedRect(0, 16, 4, height() - 32, 2, 2);
        p.fillPath(accent, QColor(0x00, 0xd4, 0xaa));
    }
}

void DistroCard::showContextMenu(const QPoint &pos)
{
    Q_UNUSED(pos);
    if (m_distro.state != DistroState::Stopped) {
        return;
    }

    QMenu menu(this);
    QAction *deleteAct = menu.addAction(QString("删除 '%1'").arg(m_distro.name));
    deleteAct->setIcon(QIcon(":/icons/window_close.svg"));

    // 将菜单显示在当前卡片的中心位置，而不是跟着鼠标
    QPoint centerPos = mapToGlobal(rect().center());
    QSize menuSize = menu.sizeHint();
    centerPos.rx() -= menuSize.width() / 2;
    centerPos.ry() -= menuSize.height() / 2;

    QAction *selected = menu.exec(centerPos);
    if (selected == deleteAct) {
        emit deleteRequested(m_distro);
    }
}


