#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include <QDir>
#include <QSharedMemory>
#include <QLocalServer>
#include <QLocalSocket>
#include <QByteArray>
#include <QDataStream>
#include <windows.h>
#include <shellapi.h>

#include "mainwindow.h"
#include "ui/disclaimerdialog.h"
#include "ui/elevationdialog.h"

static const char *SHARED_MEM_KEY   = "WSLTool-SingleInstance";
static const char *LOCAL_SERVER_KEY = "WSLTool-LocalServer";

// 检查是否以管理员权限运行
static bool isRunAsAdmin()
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

// 以管理员权限重启自身
static void relaunchAsAdmin()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);

    wchar_t dir[MAX_PATH];
    wcscpy_s(dir, path);
    wchar_t *lastSlash = wcsrchr(dir, L'\\');
    if (lastSlash) {
        *lastSlash = L'\0';
    }

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize       = sizeof(sei);
    sei.lpVerb       = L"runas";
    sei.lpFile       = path;
    sei.lpDirectory  = dir;
    sei.lpParameters = L"";
    sei.nShow        = SW_NORMAL;
    sei.fMask        = SEE_MASK_NOCLOSEPROCESS;
    ShellExecuteExW(&sei);
}

// 在顶层窗口中查找 MainWindow 并激活
static void raiseMainWindow()
{
    for (QWidget *w : qApp->topLevelWidgets()) {
        MainWindow *mw = qobject_cast<MainWindow*>(w);
        if (mw) {
            mw->showNormal();
            mw->activateWindow();
            mw->raise();
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    // 设置高 DPI
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName("WSL Manager");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("WSLTool");

    // === 先处理管理员权限 ===
    if (!isRunAsAdmin()) {
        ElevationDialog dlg;
        if (dlg.exec() == QDialog::Accepted && dlg.shouldElevate()) {
            relaunchAsAdmin();
        }
        return 0;
    }

    // === 再检测单例 ===
    QSharedMemory sharedMem(SHARED_MEM_KEY);
    if (!sharedMem.create(1)) {
        // 已有实例运行，发送激活信号后退出
        QLocalSocket socket;
        socket.connectToServer(LOCAL_SERVER_KEY);
        if (socket.waitForConnected(2000)) {
            socket.write("activate");
            socket.waitForBytesWritten(1000);
            socket.disconnectFromServer();
        }
        return 0;
    }

    // === 本实例是第一个，创建本地服务器接收激活信号 ===
    QLocalServer::removeServer(LOCAL_SERVER_KEY);
    QLocalServer *actServer = new QLocalServer;
    actServer->listen(LOCAL_SERVER_KEY);

    QObject::connect(actServer, &QLocalServer::newConnection, [actServer]() {
        while (actServer->hasPendingConnections()) {
            QLocalSocket *client = actServer->nextPendingConnection();
            QObject::connect(client, &QLocalSocket::readyRead, [client]() {
                QByteArray data = client->readAll();
                if (data.trimmed() == "activate") {
                    raiseMainWindow();
                }
                client->deleteLater();
            });
            QObject::connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
        }
    });

    // 加载全局样式表
    QFile qssFile(":/styles/styles/light_theme.qss");
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(qssFile.readAll());
        qssFile.close();
    }

    // 加载字体
    QFontDatabase::addApplicationFont(":/fonts/SegoeUI.ttf");
    QFont defaultFont("Segoe UI", 10);
    defaultFont.setHintingPreference(QFont::PreferFullHinting);
    app.setFont(defaultFont);

    // 免责声明（首次运行显示）
    if (DisclaimerDialog::shouldShow()) {
        DisclaimerDialog disclaimer;
        if (disclaimer.exec() != QDialog::Accepted) {
            return 0;
        }
    }

    MainWindow w;
    w.show();

    int ret = app.exec();

    // 清理
    delete actServer;
    QLocalServer::removeServer(LOCAL_SERVER_KEY);

    return ret;
}
