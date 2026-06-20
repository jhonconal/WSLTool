#pragma once
#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFutureWatcher>
#include <QProcess>
#include "../models/onlinedistribution.h"
#include "../models/wsldistribution.h"
#include "../models/diskinfo.h"

class OnlineDistroPage : public QWidget
{
    Q_OBJECT
public:
    explicit OnlineDistroPage(QWidget *parent = nullptr);
    ~OnlineDistroPage();

    void loadDataAsync(const QList<WslDistribution> &installedDistros,
                      const QList<DiskInfo> &disks);

private slots:
    void onFetchFinished();
    void onInstallRequested(const OnlineDistribution &distro);

signals:
    void refreshNeeded();

private:
    void setupUi();
    void populateList();

    QList<WslDistribution>   m_installedDistros;
    QList<DiskInfo>          m_disks;
    QList<OnlineDistribution> m_onlineDistros;

    QWidget     *m_listArea;
    QLabel      *m_emptyLabel;
    QLabel      *m_loadingLabel;
    QScrollArea *m_scrollArea;

    QFutureWatcher<QList<OnlineDistribution>> *m_watcher;
};
