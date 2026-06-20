#pragma once
#include <QDialog>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include "../models/onlinedistribution.h"
#include "../models/diskinfo.h"

class InstallDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InstallDialog(const OnlineDistribution &distro, const QList<DiskInfo> &disks, QWidget *parent = nullptr);

    bool isDefaultInstall() const;
    QString targetPath() const;

    bool createAccount() const;
    QString username() const;
    QString password() const;

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

    // 账号配置
    QCheckBox    *m_configAccountCheck;
    QWidget      *m_accountSection;
    QLineEdit    *m_userEdit;
    QLineEdit    *m_passEdit;
    QLineEdit    *m_pass2Edit;
};
