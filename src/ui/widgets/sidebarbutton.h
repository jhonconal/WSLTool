#pragma once
#include <QPushButton>
#include <QString>
#include <QIcon>

class SidebarButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked)
public:
    explicit SidebarButton(const QString &iconPrefix, const QString &text, QWidget *parent = nullptr);
    void setChecked(bool checked);
    bool isChecked() const;
    void updateTheme(bool isDark);

private:
    bool m_checked = false;
    QString m_iconPrefix;
    QString m_text;
    QIcon m_normalIcon;
    QIcon m_activeIcon;
    void updateStyle();
};

