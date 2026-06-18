#pragma once
#include <QWidget>
#include <QString>

class DiskUsageBar : public QWidget
{
    Q_OBJECT
public:
    explicit DiskUsageBar(QWidget *parent = nullptr);
    void setDiskInfo(const QString &letter, const QString &label,
                     qint64 used, qint64 total, const QString &freeStr);
    void updateTheme(bool isDark);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    QString m_letter;
    QString m_label;
    qint64  m_used  = 0;
    qint64  m_total = 1;
    QString m_freeStr;
    double  m_percent = 0.0;
    bool    m_isDark = false;
};

