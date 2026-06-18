#include "infocard.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

InfoCard::InfoCard(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_accentColor("#00d4aa")
{
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("infoCard");
    setMinimumWidth(180);

    QHBoxLayout *hl = new QHBoxLayout(this);
    hl->setContentsMargins(18, 16, 18, 16);
    hl->setSpacing(14);

    m_iconLabel = new QLabel;
    m_iconLabel->setObjectName("cardIcon");
    m_iconLabel->setFixedSize(40, 40);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *vl = new QVBoxLayout;
    vl->setSpacing(2);
    vl->setContentsMargins(0, 0, 0, 0);

    m_titleLabel = new QLabel(title);
    m_titleLabel->setObjectName("cardTitle");

    m_valueLabel = new QLabel("—");
    m_valueLabel->setObjectName("cardValue");

    m_subLabel = new QLabel("");
    m_subLabel->setObjectName("cardSub");
    m_subLabel->setVisible(false);

    vl->addWidget(m_titleLabel);
    vl->addWidget(m_valueLabel);
    vl->addWidget(m_subLabel);

    hl->addWidget(m_iconLabel);
    hl->addLayout(vl, 1);

    updateStyle();
}

void InfoCard::setValue(const QString &value)
{
    m_valueLabel->setText(value);
}

void InfoCard::setSubValue(const QString &sub)
{
    m_subLabel->setText(sub);
    m_subLabel->setVisible(!sub.isEmpty());
}

void InfoCard::setIcon(const QString &iconPath)
{
    QPixmap pix(iconPath);
    if (!pix.isNull()) {
        m_iconLabel->setPixmap(pix.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void InfoCard::setAccentColor(const QString &color)
{
    m_accentColor = color;
    updateStyle();
}

void InfoCard::updateStyle()
{
    m_iconLabel->setStyleSheet(QString(
        "background: rgba(%1, 0.15); border-radius: 10px;")
        .arg(m_accentColor == "#00d4aa" ? "0,212,170" :
             m_accentColor == "#f59e0b" ? "245,158,11" :
             m_accentColor == "#6366f1" ? "99,102,241" : "0,212,170"));
}

