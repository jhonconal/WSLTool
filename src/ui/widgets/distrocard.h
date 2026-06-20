#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include "../../models/wsldistribution.h"

class DistroCard : public QWidget
{
    Q_OBJECT
public:
    explicit DistroCard(const WslDistribution &distro, QWidget *parent = nullptr);

signals:
    void migrateRequested(const WslDistribution &distro);
    void deleteRequested(const WslDistribution &distro);
    void refreshNeeded();

private slots:
    void showContextMenu(const QPoint &pos);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    WslDistribution m_distro;
    QLabel *m_iconLabel;
    QLabel *m_nameLabel;
    QLabel *m_pathLabel;
    QLabel *m_sizeLabel;
    QLabel *m_diskBadge;
    QLabel *m_versionBadge;
    QLabel *m_stateBadge;
    QPushButton *m_migrateBtn;
    QPushButton *m_launchBtn;
    QPushButton *m_homeBtn;
    bool m_hovered = false;
    void setupUi();
};

