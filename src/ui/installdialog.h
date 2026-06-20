#pragma once
#include <QDialog>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../models/onlinedistribution.h"
#include "../models/diskinfo.h"

class InstallDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InstallDialog(const OnlineDistribution &distro, const QList<DiskInfo> &disks, QWidget *parent = nullptr);

    bool isDefaultInstall() const;
    QString targetPath() const;

private slots:
    void onInstallTypeChanged();
    void onBrowse();
    void validateForm();
    void onStartInstall();

private:
    void setupUi();

    OnlineDistribution m_distro;
    QList<DiskInfo>    m_disks;

    QRadioButton *m_defaultRadio;
    QRadioButton *m_customRadio;
    QLineEdit    *m_pathEdit;
    QPushButton  *m_browseBtn;
    QPushButton  *m_startBtn;
    QLabel       *m_spaceLabel;
    QWidget      *m_customSection;
};
