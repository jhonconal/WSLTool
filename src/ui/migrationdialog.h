#pragma once
#include <QDialog>
#include <QList>
#include "../models/wsldistribution.h"
#include "../models/diskinfo.h"

class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;
class QCheckBox;

class MigrationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MigrationDialog(const WslDistribution &distro,
                              const QList<DiskInfo> &disks,
                              QWidget *parent = nullptr);

private slots:
    void onDiskChanged(int index);
    void onBrowse();
    void onStartMigration();
    void validateForm();

private:
    void setupUi();
    void updateSpaceCheck();

    WslDistribution        m_distro;
    QList<DiskInfo>        m_disks;

    QComboBox  *m_diskCombo;
    QLineEdit  *m_pathEdit;
    QLabel     *m_spaceLabel;
    QLabel     *m_sizeLabel;
    QCheckBox  *m_configAccountCheck;
    QLineEdit  *m_userEdit;
    QLineEdit  *m_passEdit;
    QLineEdit  *m_pass2Edit;
    QPushButton *m_startBtn;
    QWidget    *m_accountSection;
};

