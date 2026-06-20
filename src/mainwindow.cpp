#include "mainwindow.h"
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#endif
#include <QCursor>
#include "core/systemdetector.h"
#include "core/wslmanager.h"
#include "core/diskmanager.h"
#include "ui/dashboardpage.h"
#include "ui/distributionpage.h"
#include "ui/widgets/sidebarbutton.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QSvgRenderer>
#include <QFile>
#include <QSettings>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setMinimumSize(800, 600);
    resize(1280, 768);

    QSettings settings("WSLTool", "Preferences");
    m_isDarkTheme = settings.value("darkTheme", false).toBool();

    setupUi();
    applyStyle();
    applyTheme();
    setupTrayIcon();
    loadData();
    switchPage(0);

    // 居中显示
    QScreen *screen = QApplication::primaryScreen();
    QRect sg = screen->availableGeometry();
    move(sg.center() - rect().center());
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    central->setObjectName("centralWidget");
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---- 标题栏 ----
    setupTitleBar();
    mainLayout->addWidget(m_titleBar);

    // ---- 主体（侧边栏 + 内容）----
    QWidget *body = new QWidget;
    body->setObjectName("bodyWidget");
    QHBoxLayout *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    setupSidebar();
    bodyLayout->addWidget(m_sidebar);

    setupContent();
    bodyLayout->addWidget(m_stack, 1);

    mainLayout->addWidget(body, 1);
}

void MainWindow::setupTitleBar()
{
    m_titleBar = new QWidget;
    m_titleBar->setObjectName("titleBar");
    m_titleBar->setFixedHeight(52);

    QHBoxLayout *tl = new QHBoxLayout(m_titleBar);
    tl->setContentsMargins(20, 0, 12, 0);
    tl->setSpacing(12);

    // Logo icon + 标题
    QLabel *icon = new QLabel;
    QPixmap logoPix(":/icons/tux.svg");
    if (!logoPix.isNull()) {
        icon->setPixmap(logoPix.scaled(22, 22, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    //m_titleLabel = new QLabel("WSL 管理工具");
    m_titleLabel = new QLabel("WSL Tool");
    m_titleLabel->setObjectName("titleLabel");

    tl->addWidget(icon);
    tl->addWidget(m_titleLabel);
    tl->addStretch();

    // 刷新按钮
    m_refreshBtn = new QPushButton("刷新");
    m_refreshBtn->setObjectName("refreshBtn");
    m_refreshBtn->setFixedHeight(36);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshData);
    tl->addWidget(m_refreshBtn);

    // 主题切换按钮
    m_themeBtn = new QPushButton;
    m_themeBtn->setObjectName("themeBtn");
    m_themeBtn->setFixedSize(36, 36);
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_themeBtn, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    tl->addWidget(m_themeBtn);

    // 窗口控制按钮
    auto mkBtn = [&](const QString &id) {
        QPushButton *b = new QPushButton;
        b->setObjectName(id);
        b->setFixedSize(36, 36);
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };

    QPushButton *minBtn   = mkBtn("minBtn");
    m_maxBtn              = mkBtn("maxBtn");
    QPushButton *closeBtn = mkBtn("closeBtn");

    connect(minBtn,   &QPushButton::clicked, this, &MainWindow::onMinimize);
    connect(m_maxBtn, &QPushButton::clicked, this, &MainWindow::onMaximize);
    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::onClose);

    tl->addWidget(minBtn);
    tl->addWidget(m_maxBtn);
    tl->addWidget(closeBtn);
}

void MainWindow::setupSidebar()
{
    m_sidebar = new QWidget;
    m_sidebar->setObjectName("sidebar");
    m_sidebar->setFixedWidth(200);

    QVBoxLayout *sl = new QVBoxLayout(m_sidebar);
    sl->setContentsMargins(12, 20, 12, 20);
    sl->setSpacing(4);

    struct { QString icon; QString text; } pages[] = {
        {"home", "仪表盘"},
        {"distro", "发行版管理"},
    };

    for (int i = 0; i < 2; i++) {
        SidebarButton *btn = new SidebarButton(pages[i].icon, pages[i].text, this);
        btn->setChecked(i == 0);
        connect(btn, &SidebarButton::clicked, this, [this, i, btn]() {
            switchPage(i);
            for (auto *b : m_sidebarBtns) b->setChecked(false);
            btn->setChecked(true);
        });
        m_sidebarBtns.append(btn);
        sl->addWidget(btn);
    }

    sl->addStretch();

    // 版本信息
    QLabel *ver = new QLabel("WSL Tool v1.0");
    ver->setObjectName("versionLabel");
    ver->setStyleSheet("font-size: 14px; font-weight: bold;");
    ver->setAlignment(Qt::AlignCenter);
    sl->addWidget(ver);
}

void MainWindow::setupContent()
{
    m_stack = new QStackedWidget;
    m_stack->setObjectName("contentStack");

    m_dashboardPage = new DashboardPage(this);
    m_distroPage    = new DistributionPage(this);

    m_stack->addWidget(m_dashboardPage);
    m_stack->addWidget(m_distroPage);
}

void MainWindow::loadData()
{
    m_refreshBtn->setEnabled(false);
    m_refreshBtn->setText("加载中...");

    SystemDetector sd;
    m_sysInfo = sd.detect();

    WslManager wm;
    m_distros = wm.enumerateDistributions();

    DiskManager dm;
    m_disks = dm.enumerateFixedDisks();

    m_dashboardPage->setData(m_sysInfo, m_distros, m_disks);
    m_distroPage->setData(m_distros, m_disks);

    // 重建的控件需要同步当前主题
    m_dashboardPage->updateTheme(m_isDarkTheme);

    m_refreshBtn->setEnabled(true);
    m_refreshBtn->setText("刷新");
}

void MainWindow::refreshData()
{
    loadData();
}

void MainWindow::switchPage(int index)
{
    m_stack->setCurrentIndex(index);
}

void MainWindow::onMinimize()
{
    showMinimized();
}

void MainWindow::onMaximize()
{
    if (m_maximized) {
        showNormal();
        m_maximized = false;
    } else {
        showMaximized();
        m_maximized = true;
    }
    updateIcons();
}

void MainWindow::onClose()
{
    hide();
    m_trayIcon->show();
#if 0
    m_trayIcon->showMessage(
        "WSL Tool",
        "程序已最小化到系统托盘。",
        QSystemTrayIcon::Information,
    3000);
#endif
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    m_trayIcon->show();
    event->ignore();
}

void MainWindow::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/tux.svg"));
    m_trayIcon->setToolTip("WSL Tool");

    m_trayMenu = new QMenu(this);

    QAction *showAction = m_trayMenu->addAction(QIcon(":/icons/home.svg"), "显示主页面");
    connect(showAction, &QAction::triggered, this, [this]() {
        showNormal();
        activateWindow();
        raise();
        m_trayIcon->hide();
    });

    m_trayMenu->addSeparator();

    QAction *exitAction = m_trayMenu->addAction(QIcon(":/icons/window_close.svg"), "关闭退出程序");
    connect(exitAction, &QAction::triggered, this, [this]() {
        m_trayIcon->hide();
        QApplication::quit();
    });

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
        [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick) {
                showNormal();
                activateWindow();
                raise();
                m_trayIcon->hide();
            }
        });
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 只在标题栏区域内拖拽
        if (m_titleBar->geometry().contains(event->pos())) {
            m_dragging = true;
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPosition);
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
}

void MainWindow::applyStyle()
{
    // 透明背景等由 QSS 的 centralWidget 统一处理
}

void MainWindow::toggleTheme()
{
    m_isDarkTheme = !m_isDarkTheme;
    applyTheme();

    QSettings settings("WSLTool", "Preferences");
    settings.setValue("darkTheme", m_isDarkTheme);
}

void MainWindow::applyTheme()
{
    QString qssPath = m_isDarkTheme ? ":/styles/styles/dark_theme.qss" : ":/styles/styles/light_theme.qss";
    QFile file(qssPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString qss = file.readAll();
        qApp->setStyleSheet(qss);
        file.close();
    }

    QIcon themeIcon;
    if (m_isDarkTheme) {
        themeIcon = loadThemeIcon(":/icons/theme_light.svg", "#94a3b8");
    } else {
        themeIcon = loadThemeIcon(":/icons/theme_dark.svg", "#475569");
    }
    m_themeBtn->setIcon(themeIcon);
    m_themeBtn->setIconSize(QSize(18, 18));

    updateIcons();

    if (m_dashboardPage) {
        m_dashboardPage->updateTheme(m_isDarkTheme);
    }
}

void MainWindow::updateIcons()
{
    QString normalColor = m_isDarkTheme ? "#94a3b8" : "#475569";
    QString hoverColor  = m_isDarkTheme ? "#e2e8f0" : "#0f172a";

    QPushButton *minBtn = findChild<QPushButton*>("minBtn");
    if (minBtn) {
        minBtn->setIcon(loadThemeIcon(":/icons/window_minimize.svg", normalColor, hoverColor));
    }
    if (m_maxBtn) {
        m_maxBtn->setIcon(loadThemeIcon(m_maximized ? ":/icons/window_restore.svg" : ":/icons/window_maximize.svg", normalColor, hoverColor));
    }
    QPushButton *closeBtn = findChild<QPushButton*>("closeBtn");
    if (closeBtn) {
        closeBtn->setIcon(loadThemeIcon(":/icons/window_close.svg", normalColor, "#ff4d6d"));
    }

    QString tealColor = m_isDarkTheme ? "#00d4aa" : "#008f72";
    m_refreshBtn->setIcon(loadThemeIcon(":/icons/refresh.svg", tealColor));

    for (SidebarButton *btn : m_sidebarBtns) {
        btn->updateTheme(m_isDarkTheme);
    }
}

QIcon MainWindow::loadThemeIcon(const QString &resourcePath, const QString &normalColor, const QString &activeColor)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) return QIcon(resourcePath);
    QByteArray data = file.readAll();
    file.close();

    QByteArray normalData = data;
    normalData.replace("#212121", normalColor.toUtf8());
    normalData.replace("#64748b", normalColor.toUtf8());

    QIcon icon;

    QSvgRenderer rendererNormal(normalData);
    QPixmap pixNormal(24, 24);
    pixNormal.fill(Qt::transparent);
    {
        QPainter p(&pixNormal);
        rendererNormal.render(&p);
    }
    icon.addPixmap(pixNormal, QIcon::Normal);

    if (!activeColor.isEmpty()) {
        QByteArray activeData = data;
        activeData.replace("#212121", activeColor.toUtf8());
        activeData.replace("#64748b", activeColor.toUtf8());

        QSvgRenderer rendererActive(activeData);
        QPixmap pixActive(24, 24);
        pixActive.fill(Qt::transparent);
        {
            QPainter p(&pixActive);
            rendererActive.render(&p);
        }
        icon.addPixmap(pixActive, QIcon::Active);
    }

    return icon;
}

#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_NCHITTEST) {
        // 使用 QCursor::pos() 获取 Qt 逻辑坐标，避免高 DPI 缩放带来的物理像素偏移问题
        QPoint globalPos = QCursor::pos();
        QPoint localPos = mapFromGlobal(globalPos);

        int width = this->width();
        int height = this->height();
        const int border = 8; // 边缘检测的宽度，8像素比较容易触发

        bool left = localPos.x() < border;
        bool right = localPos.x() > width - border;
        bool top = localPos.y() < border;
        bool bottom = localPos.y() > height - border;

        // 最大化状态下不能拉伸
        if (m_maximized) {
            return false; // 交给 Qt 默认处理
        }

        if (left && top) {
            *result = HTTOPLEFT;
            return true;
        } else if (left && bottom) {
            *result = HTBOTTOMLEFT;
            return true;
        } else if (right && top) {
            *result = HTTOPRIGHT;
            return true;
        } else if (right && bottom) {
            *result = HTBOTTOMRIGHT;
            return true;
        } else if (left) {
            *result = HTLEFT;
            return true;
        } else if (right) {
            *result = HTRIGHT;
            return true;
        } else if (top) {
            *result = HTTOP;
            return true;
        } else if (bottom) {
            *result = HTBOTTOM;
            return true;
        }
    }
    return false;
}
#endif

