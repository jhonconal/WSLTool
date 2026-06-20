#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>

#include "models/systeminfo.h"
#include "models/wsldistribution.h"
#include "models/diskinfo.h"
#include "models/onlinedistribution.h"

class DashboardPage;
class DistributionPage;
class OnlineDistroPage;
class SidebarButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static QIcon loadThemeIcon(const QString &resourcePath, const QString &normalColor, const QString &activeColor = "");

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

private slots:
    void switchPage(int index);
    void refreshData();
    void onMinimize();
    void onMaximize();
    void onClose();
    void toggleTheme();

private:
    void setupUi();
    void setupTitleBar();
    void setupSidebar();
    void setupContent();
    void applyStyle();
    void loadData();
    void applyTheme();
    void updateIcons();
    void setupTrayIcon();

    // Title bar drag
    QPoint m_dragPosition;
    bool   m_dragging = false;
    bool   m_maximized = false;
    bool   m_isDarkTheme = false;

    // Widgets
    QWidget        *m_titleBar;
    QWidget        *m_sidebar;
    QStackedWidget *m_stack;
    DashboardPage  *m_dashboardPage;
    DistributionPage *m_distroPage;
    OnlineDistroPage *m_onlineDistroPage;

    QList<SidebarButton*> m_sidebarBtns;
    QLabel *m_titleLabel;
    QPushButton *m_refreshBtn;
    QPushButton *m_maxBtn;
    QPushButton *m_themeBtn;

    // Data
    SystemInfo              m_sysInfo;
    QList<WslDistribution>  m_distros;
    QList<DiskInfo>         m_disks;
    QList<OnlineDistribution> m_onlineDistros;

    // System tray
    QSystemTrayIcon *m_trayIcon;
    QMenu           *m_trayMenu;
};

