#include "sidebarbutton.h"
#include "../../mainwindow.h"
#include <QStyle>
#include <QVariant>

SidebarButton::SidebarButton(const QString &iconPrefix, const QString &text, QWidget *parent)
    : QPushButton(parent), m_iconPrefix(iconPrefix), m_text(text)
{
    setText("  " + text);
    setFixedHeight(44);
    setCursor(Qt::PointingHandCursor);
    setCheckable(false);
    setIconSize(QSize(18, 18));

    updateTheme(true);
}

void SidebarButton::updateTheme(bool isDark)
{
    QString normalColor = isDark ? "#64748b" : "#475569";
    QString activeColor = isDark ? "#00d4aa" : "#008f72";

    m_normalIcon = MainWindow::loadThemeIcon(QString(":/icons/%1.svg").arg(m_iconPrefix), normalColor);
    m_activeIcon = MainWindow::loadThemeIcon(QString(":/icons/%1.svg").arg(m_iconPrefix), activeColor);

    setProperty("theme", isDark ? "dark" : "light");
    updateStyle();
}

void SidebarButton::setChecked(bool checked)
{
    if (m_checked == checked) return;
    m_checked = checked;
    updateStyle();
}

bool SidebarButton::isChecked() const { return m_checked; }

void SidebarButton::updateStyle()
{
    setIcon(m_checked ? m_activeIcon : m_normalIcon);
    setProperty("checked", m_checked);
    
    style()->unpolish(this);
    style()->polish(this);
    update();
}

