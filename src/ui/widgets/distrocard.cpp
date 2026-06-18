#include "distrocard.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QVariant>

DistroCard::DistroCard(const WslDistribution &distro, QWidget *parent)
    : QWidget(parent), m_distro(distro)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumHeight(100);
    setMaximumHeight(120);
    setCursor(Qt::ArrowCursor);
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

