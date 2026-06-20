#pragma once
#include <QWidget>
#include <QLabel>
#include <QList>
#include "../models/wsldistribution.h"
#include "../models/diskinfo.h"

class DistributionPage : public QWidget
{
    Q_OBJECT
public:
    explicit DistributionPage(QWidget *parent = nullptr);
    void setData(const QList<WslDistribution> &distros,
                 const QList<DiskInfo> &disks);

signals:
    void refreshNeeded();

private slots:
    void onMigrateRequested(const WslDistribution &distro);
    void onDeleteRequested(const WslDistribution &distro);

private:
    void setupUi();
    void populateList();

    QList<WslDistribution> m_distros;
    QList<DiskInfo>        m_disks;
    QWidget               *m_listArea;
    QLabel                *m_emptyLabel;
};

