#pragma once
#include <QWidget>
#include <QLabel>

class InfoCard : public QWidget
{
    Q_OBJECT
public:
    explicit InfoCard(const QString &title, QWidget *parent = nullptr);
    void setValue(const QString &value);
    void setSubValue(const QString &sub);
    void setIcon(const QString &iconPath);
    void setAccentColor(const QString &color);

private:
    QLabel *m_titleLabel;
    QLabel *m_valueLabel;
    QLabel *m_subLabel;
    QLabel *m_iconLabel;
    QString m_accentColor;
    void updateStyle();
};

