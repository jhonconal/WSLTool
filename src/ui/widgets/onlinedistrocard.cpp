#include "onlinedistrocard.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QVariant>
#include <QStyle>

OnlineDistroCard::OnlineDistroCard(const OnlineDistribution &distro, QWidget *parent)
    : QWidget(parent), m_distro(distro)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumHeight(90);
    setMaximumHeight(110);
    setupUi();
}

void OnlineDistroCard::setupUi()
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

    // 第一行：友好名称 + 状态 badge
    QHBoxLayout *row1 = new QHBoxLayout;
    row1->setSpacing(12);

    m_friendlyLabel = new QLabel(m_distro.friendlyName);
    m_friendlyLabel->setObjectName("distroName");
    row1->addWidget(m_friendlyLabel);

    m_statusBadge = new QLabel(m_distro.isInstalled ? "已安装" : "可安装");
    m_statusBadge->setObjectName("distroStateBadge");
    m_statusBadge->setProperty("state", m_distro.isInstalled ? "running" : "stopped");
    row1->addWidget(m_statusBadge);
    row1->addStretch();

    // 第二行：标识名称
    m_nameLabel = new QLabel("标识名称: " + m_distro.name);
    m_nameLabel->setObjectName("distroPath"); // 借用路径样式（小字灰色）

    vl->addLayout(row1);
    vl->addWidget(m_nameLabel);

    // 安装按钮
    m_installBtn = new QPushButton(m_distro.isInstalled ? "已安装" : "安装");
    m_installBtn->setObjectName("distroInstallBtn");
    m_installBtn->setFixedSize(90, 36);
    m_installBtn->setCursor(Qt::PointingHandCursor);
    m_installBtn->setEnabled(!m_distro.isInstalled);

    connect(m_installBtn, &QPushButton::clicked, this, [this]() {
        emit installRequested(m_distro);
    });

    hl->addWidget(m_iconLabel);
    hl->addLayout(vl, 1);
    hl->addWidget(m_installBtn);
}

void OnlineDistroCard::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void OnlineDistroCard::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

void OnlineDistroCard::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    if (m_hovered) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath accent;
        accent.addRoundedRect(0, 16, 4, height() - 32, 2, 2);
        p.fillPath(accent, QColor(0x00, 0xd4, 0xaa));
    }
}
