#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include "../../models/onlinedistribution.h"

class OnlineDistroCard : public QWidget
{
    Q_OBJECT
public:
    explicit OnlineDistroCard(const OnlineDistribution &distro, QWidget *parent = nullptr);

signals:
    void installRequested(const OnlineDistribution &distro);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    OnlineDistribution m_distro;
    QLabel *m_iconLabel;
    QLabel *m_nameLabel;
    QLabel *m_friendlyLabel;
    QLabel *m_statusBadge;
    QPushButton *m_installBtn;
    bool m_hovered = false;

    void setupUi();
};
