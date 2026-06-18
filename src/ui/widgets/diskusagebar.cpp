#include "diskusagebar.h"
#include <QPainter>
#include <QPainterPath>

DiskUsageBar::DiskUsageBar(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(72);
    setMaximumHeight(90);
}

void DiskUsageBar::setDiskInfo(const QString &letter, const QString &label,
                                qint64 used, qint64 total, const QString &freeStr)
{
    m_letter   = letter;
    m_label    = label;
    m_used     = used;
    m_total    = total > 0 ? total : 1;
    m_freeStr  = freeStr;
    m_percent  = (double)used / m_total * 100.0;
    update();
}

QSize DiskUsageBar::sizeHint() const { return {220, 80}; }

void DiskUsageBar::updateTheme(bool isDark)
{
    m_isDark = isDark;
    update();
}

void DiskUsageBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int W = width(), H = height();
    int padX = 12, padY = 8;

    // 根据主题选择配色
    QColor cardBg = m_isDark ? QColor(0x1a, 0x1d, 0x2e) : QColor(0xff, 0xff, 0xff);
    QColor cardBorder = m_isDark ? QColor(255, 255, 255, 18) : QColor(0, 0, 0, 15);
    QColor titleColor = m_isDark ? QColor(0xe2, 0xe8, 0xf0) : QColor(0x1f, 0x29, 0x37);
    QColor textColor = m_isDark ? QColor(0x64, 0x74, 0x8b) : QColor(0x47, 0x55, 0x69);
    QColor barBg = m_isDark ? QColor(0x0f, 0x11, 0x17) : QColor(0xe5, 0xe7, 0xeb);

    // 背景卡片
    p.setPen(QPen(cardBorder, 1));
    p.setBrush(cardBg);
    QPainterPath bg;
    bg.addRoundedRect(0, 0, W, H, 12, 12);
    p.drawPath(bg);

    // 盘符标题
    p.setPen(titleColor);
    p.setFont(QFont("Segoe UI", 13, QFont::Bold));
    p.drawText(padX, padY + 16, m_letter + ":");

    // 标签/剩余
    p.setPen(textColor);
    p.setFont(QFont("Segoe UI", 9));
    QString sub = m_label.isEmpty() ? m_freeStr + " 可用" : m_label + "  " + m_freeStr + " 可用";
    p.drawText(padX + 30, padY + 16, sub);

    // 百分比
    QString pctStr = QString::number(m_percent, 'f', 1) + "%";
    p.setPen(m_percent > 85 ? QColor(0xff, 0x4d, 0x6d) : (m_isDark ? QColor(0x00, 0xd4, 0xaa) : QColor(0x00, 0x8f, 0x72)));
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));
    p.drawText(W - padX - 40, padY + 16, pctStr);

    // 进度条背景
    int barY = padY + 26;
    int barH = 8;
    int barW = W - padX * 2;
    QPainterPath bgBar;
    bgBar.addRoundedRect(padX, barY, barW, barH, barH/2, barH/2);
    p.setBrush(barBg);
    p.setPen(Qt::NoPen);
    p.drawPath(bgBar);

    // 进度条前景
    int fillW = qMax(barH, (int)(barW * m_percent / 100.0));
    QPainterPath fgBar;
    fgBar.addRoundedRect(padX, barY, fillW, barH, barH/2, barH/2);

    QLinearGradient grad(padX, 0, padX + fillW, 0);
    if (m_percent > 85) {
        grad.setColorAt(0, QColor(0xff, 0x6b, 0x35));
        grad.setColorAt(1, QColor(0xff, 0x4d, 0x6d));
    } else if (m_percent > 65) {
        grad.setColorAt(0, QColor(0xf5, 0x9e, 0x0b));
        grad.setColorAt(1, QColor(0xfb, 0xbd, 0x23));
    } else {
        grad.setColorAt(0, m_isDark ? QColor(0x00, 0xd4, 0xaa) : QColor(0x00, 0xa8, 0x85));
        grad.setColorAt(1, m_isDark ? QColor(0x00, 0xb4, 0xd8) : QColor(0x00, 0x8d, 0xa5));
    }
    p.setBrush(grad);
    p.drawPath(fgBar);

    // 使用量文字
    p.setPen(textColor);
    p.setFont(QFont("Segoe UI", 8));
    auto fmt = [](qint64 b) -> QString {
        if (b >= (qint64)1<<30) return QString::number(b/(1<<30)) + "G";
        return QString::number(b/(1<<20)) + "M";
    };
    p.drawText(padX, barY + barH + 14, fmt(m_used) + " / " + fmt(m_total));
}

