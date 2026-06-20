# AGENTS.md — AI 协作开发指南

本文档面向所有参与此项目的 AI 编码助手（Agent），描述项目架构、编码规范、禁止操作与推荐工作流。

---

## 项目概览

**WSLTool** 是一个 Windows 桌面 GUI 应用，使用 **Qt 5.15.2 + C++17 + MinGW 8.1.0** 构建。

- **目标平台**：Windows 7 SP1+ (x64)，需要 UAC 管理员权限
- **核心功能**：WSL 发行版的图形化管理与磁盘间迁移
- **构建系统**：qmake（`WSLTool.pro`） + `build.bat` 一键脚本
- **打包工具**：windeployqt + Inno Setup 6（`installer/WSLTool_Setup.iss`）

---

## 代码架构

```
src/
├── main.cpp                     # 入口：单例检测(QLocalServer)、UAC 提权检测(ElevationDialog)、QSS 主题、字体、DisclaimerDialog
├── mainwindow.h/cpp             # 主窗口：无边框拖拽、侧边栏路由、数据加载、系统托盘(QSystemTrayIcon)
├── core/                        # 纯业务逻辑，无 UI 依赖
│   ├── wslmanager               # 注册表枚举 WSL 发行版、调用 wsl.exe
│   ├── migrationworker          # 迁移任务（QThread 子类，分步执行）
│   ├── installworker            # 在线安装与账户配置（QThread 子类，分步执行）
│   ├── systemdetector           # Windows 版本、WSL 版本检测
│   └── diskmanager              # GetDiskFreeSpaceEx / WMI 磁盘信息
├── models/                      # 纯数据结构头文件，无 Qt 对象
│   ├── wsldistribution.h        # WslDistribution struct + WslVersion/DistroState enum
│   ├── onlinedistribution.h     # OnlineDistribution struct (在线发行版模型)
│   ├── diskinfo.h               # DiskInfo struct
│   └── systeminfo.h             # SystemInfo struct
└── ui/                          # Qt Widgets UI 层
    ├── dashboardpage            # 首页：SystemInfo + DiskInfo 展示
    ├── distributionpage         # 发行版列表：DistroCard 网格（支持右键删除、终端启动目录切换）
    ├── onlinedistropage         # 在线发行版列表页面
    ├── installdialog            # 在线安装配置对话框（用户名、密码、指定目录）
    ├── installprogressdialog    # 在线安装进度页面（实时日志、屏蔽 ESC 键）
    ├── migrationdialog          # 迁移向导：步骤 1 配置
    ├── migrationprogressdialog  # 迁移向导：步骤 2 实时进度（屏蔽 ESC 键）
    ├── disclaimerdialog         # 首次运行免责声明（QSettings 持久化状态）
    ├── elevationdialog          # 管理员权限申请（自定义 UI，替代原生 MessageBoxW）
    └── widgets/
        ├── distrocard           # 单个发行版卡片（支持迁移、启动/停止切换、主目录启动、右键删除）
        ├── onlinedistrocard     # 在线发行版卡片组件
        ├── diskusagebar         # 磁盘使用率可视化条（paintEvent 自定义绘制，支持深/浅主题）
        ├── infocard             # 通用信息卡片
        └── sidebarbutton        # 侧边栏导航按钮（含选中态）
```

### 数据流

```
MainWindow::loadData()
  └─► SystemDetector::detect()       → SystemInfo
  └─► WslManager::enumerateDistributions() → QList<WslDistribution>
  └─► DiskManager::getDisks()        → QList<DiskInfo>
        │
        ▼
  DashboardPage  ← SystemInfo + QList<DiskInfo>
  DistributionPage ← QList<WslDistribution>
        │
        ▼ (用户点击迁移)
  MigrationDialog (配置 MigrationConfig)
        │
        ▼
  MigrationProgressDialog
  └─► MigrationWorker (QThread)
        signals: stepChanged / logMessage / progressValue / finished
```

### 单例运行机制

```
main.cpp 入口流程：

  ① 创建 QApplication
  ② 不是管理员？ → ElevationDialog → relaunchAsAdmin() → 退出
  ③ QSharedMemory::create(1) 失败？ → 已有实例 → 发 "activate" 信号 → 退出
  ④ 创建 QLocalServer 监听 "WSLTool-LocalServer"
  ⑤ 加载 QSS 主题 → 字体 → DisclaimerDialog → MainWindow → exec()
```

- `QSharedMemory` 检测是否已有实例运行
- `QLocalServer` / `QLocalSocket` 接收激活信号，将现有窗口带到前台
- 管理员提权检查在单例检测 **之前**，避免提权后的新进程被旧进程的共享内存残留误杀

### 系统托盘

```
关闭按钮 → hide() + m_trayIcon->show()
托盘右键菜单: ["显示主页面", "关闭退出程序"]
托盘双击: showNormal() + activateWindow() + raise()
Alt+F4 / 系统关闭 → closeEvent 拦截 → 隐藏到托盘而非退出
```

---

## 编码规范

### 命名约定

| 类别 | 风格 | 示例 |
|------|------|------|
| 类名 | PascalCase | `WslManager`, `ElevationDialog` |
| 成员变量 | `m_` 前缀 + camelCase | `m_distros`, `m_trayIcon` |
| 私有方法 | camelCase | `setupTrayIcon()`, `raiseMainWindow()` |
| 信号 | camelCase | `stepChanged(int, int, QString)` |
| 枚举值 | PascalCase（enum class） | `WslVersion::WSL2`, `DistroState::Running` |

### Qt 使用规范

- 所有继承 `QObject` 的类必须加 `Q_OBJECT` 宏
- 父子关系用 `parent` 参数管理内存，避免裸 `delete`
- 跨线程通信统一使用 Qt 信号槽（`Qt::QueuedConnection`）
- `QThread` 子类：重写 `run()`，不直接操作 UI
- 耗时操作（进程调用、文件 I/O）不得在主线程执行

### Windows API

- 使用 `WINVER=0x0601` / `_WIN32_WINNT=0x0601`（Windows 7 兼容）
- 注册表访问使用 `RegOpenKeyExW` / `RegQueryValueExW`（Unicode）
- 外部进程调用：使用 `QProcess` 封装，设置合理超时
  - 普通命令：`8000ms`（`SystemDetector`）
  - `wsl --export`：`600000ms`（10 分钟，`MigrationWorker`）

### 样式与主题

- **浅色主题 QSS**：`resources/styles/light_theme.qss`
- **深色主题 QSS**：`resources/styles/dark_theme.qss`
- 新增 Widget 需在两个 QSS 文件中添加对应的 objectName 选择器
- `DiskUsageBar` 使用 `paintEvent` 自定义绘制（不依赖 QSS），需通过 `updateTheme(bool)` 同步主题
- 自定义绘制控件的默认主题配色应与 `QSettings("WSLTool", "Preferences")` 的 `darkTheme` 默认值（`false`）一致
- `dark_theme.qss` 和 `light_theme.qss` 必须对称维护，缺少任何一个选择器都会导致按钮显示系统默认样式（如焦点虚线框）

---

## 关键模块说明

### `main.cpp` — 程序入口

完整流程：
1. 高 DPI 设置
2. `QApplication` 初始化
3. **管理员权限检查**：非管理员 → `ElevationDialog` 弹出 → 选择提权则 `relaunchAsAdmin()` → 退出
4. **单例检测**：`QSharedMemory::create(1)` 失败 → `QLocalSocket` 发送 `"activate"` 唤醒已有实例 → 退出
5. 创建 `QLocalServer` 监听激活信号
6. 加载浅色主题 QSS → 字体
7. `DisclaimerDialog` 首次运行检查
8. `MainWindow` 构建显示

### `ElevationDialog` — 管理员权限申请

替代原有的 `MessageBoxW`，提供与应用一致的主题风格：
- 深色/浅色主题自动适配（读取 `QSettings`）
- `shouldElevate()` 返回用户是否选择提权
- 按钮：「退出程序」/「以管理员身份重启」

### `MainWindow` — 主窗口

新增功能：
- `setupTrayIcon()`：创建系统托盘图标（使用 `tux.svg`）
- `closeEvent` 重写：拦截关闭事件，隐藏到托盘
- `onClose()`：关闭按钮隐藏到托盘
- 托盘右键菜单：「显示主页面」「关闭退出程序」
- `raiseMainWindow()` 静态函数用于单例激活

### `DiskUsageBar` — 磁盘使用率条（自定义绘制）

- 使用 `paintEvent` 纯 QPainter 绘制，不依赖 QSS
- `m_isDark` 默认值必须与应用默认主题一致（`false` = 浅色）
- `updateTheme(bool)` 由 `DashboardPage::updateTheme()` 传播
- **注意**：`loadData()` 重建控件后需手动调用 `updateTheme()` 同步当前主题

### `MigrationWorker` — 迁移核心

迁移分 6 个步骤，任一步骤失败则调用 `rollback()`：

```
stepStopDistro()    → wsl --terminate <name>
stepExport()        → wsl --export <name> <temp.tar>
stepUnregister()    → wsl --unregister <name>
stepImport()        → wsl --import <name> <targetDir> <temp.tar>
stepConfigureAccount() → wsl -d <name> -u root useradd / passwd
stepCleanup()       → 删除临时 tar 文件
```

- `m_exported` 与 `m_unregistered` 标志用于回滚判断
- 通过 `m_cancelRequested` 原子标志支持取消（在步骤间检查）
- **不可在 `run()` 中直接操作 Qt UI**，所有进度通过 `signals` 发出

### `InstallWorker` — 在线安装核心

实现在线发行版静默一键部署（带可选自定义迁移）。步骤包括：
- `stepInstallDistro()`：静默下载安装包（`wsl.exe --install <name> --no-launch`）
- `stepInitialize()`：使用 root 身份执行简易命令触发 rootfs 解压初始化（`wsl.exe -d <name> -u root -- echo initialized`）
- `stepCreateUser()`：使用 root 运行 `sh` 指令创建普通 Linux 账户并加入管理员组（支持 `sudo`/`wheel` fallback，解决无 bash 时的崩溃风险）
- `stepSetPassword()`：运行 `sh` 命令通过 `chpasswd` 静默设置账户密码
- `stepSetDefaultUser()`：通过 `printf` 写入 `/etc/wsl.conf` 配置 `[user] default = <user>` 持久化默认登录用户，并写入注册表 `DefaultUid` 双重保险
- `stepMigrate()`：如用户指定了自定义磁盘，打包导出（`wsl --export`），注销原 C 盘实例，导入（`wsl --import`）至自定义路径，并还原 `DefaultUid` 到新注册表项下，然后清理临时 tar 包

### `InstallProgressDialog` / `MigrationProgressDialog` — 实时进度监控

- 实时通过 QThread 信号显示分步流程、进度值及控制台输出日志
- **界面防误触机制**：
  - 重写了 `keyPressEvent(QKeyEvent *event)`，如果捕获到 `ESC` 键则调用 `event->ignore()` 进行拦截，防止用户意外退出导致后台任务被杀或环境配置状态不一致。
  - 重写 `closeEvent(QCloseEvent *event)` 进行拦截，仅当任务完成后或用户确认强制关闭时方可退出。
  - 优化了对话框最小尺寸和文本编辑区（QTextEdit）的高度比例，防止因为窗口被布局挤压导致 Label 控件重叠。

### `WslManager` — 发行版枚举

1. `fromRegistry()`：读取 `HKCU\Software\Microsoft\Windows\CurrentVersion\Lxss` 下所有子键
2. `mergeCommandLineInfo()`：执行 `wsl --list --verbose` 获取运行状态补充信息
3. `findVhdxPath()`：在基路径下递归查找 `ext4.vhdx`
4. 发行版显示名由 `detectDistroDisplayName()` 从注册表 `DistributionName` 字段提取

### `SystemDetector` — 环境检测

- Windows 版本：通过 `RtlGetVersion` 或注册表读取
- WSL 版本：执行 `wsl --version` 并解析输出
- 检测失败时填充默认值，不抛异常

---

## 禁止操作

在修改此项目时，以下操作 **严禁** 执行：

1. **不得在主线程调用任何阻塞 I/O 或 `wsl.exe` 进程**
   - 所有耗时操作必须在 `QThread` 或 `QtConcurrent` 中执行

2. **不得修改 `MigrationConfig` 结构体字段名**
   - 它在 `MigrationDialog` 与 `MigrationWorker` 之间传递，修改需同步两端

3. **不得绕过 UAC 检测**
   - `isRunAsAdmin()` 检查是安全必要的，不可删除或弱化

4. **不得在 `rollback()` 中调用 `stepUnregister()`**
   - 回滚时只有在已导出未注销的情况下才能安全回滚，逻辑不可随意更改

5. **不得更改 `.pro` 文件中的 `WINVER` 定义**
   - 当前为 `0x0601`（Windows 7），降低会导致 API 不可用，提高会破坏兼容性

6. **不得将 UI 代码移入 `core/` 目录**
   - `core/` 层不得依赖任何 Qt Widget 头文件

7. **不得在 `models/` 中添加 `QObject` 子类**
   - models 只包含纯数据结构（struct/enum），保持轻量无状态

8. **不得在 `dark_theme.qss` 和 `light_theme.qss` 之间不同步新增选择器**
   - 缺少对称的 QSS 选择器会导致某些控件在对应主题下显示系统默认样式

---

## 添加新功能的指南

### 新增一个 UI 页面

1. 在 `src/ui/` 创建 `yourpage.h` 和 `yourpage.cpp`
2. 在 `WSLTool.pro` 的 `SOURCES` 和 `HEADERS` 中添加路径
3. 在 `mainwindow.h` 中前向声明并添加成员指针
4. 在 `MainWindow::setupSidebar()` 中添加 `SidebarButton`
5. 在 `MainWindow::setupContent()` 中添加到 `m_stack`
6. 在 `resources/styles/dark_theme.qss` **和** `light_theme.qss` 中添加样式

### 新增一个自定义绘制控件（paintEvent）

1. 在 `src/ui/widgets/` 创建控件
2. 添加 `updateTheme(bool)` 方法用于主题切换
3. 成员变量 `m_isDark` 默认值必须与应用全局默认主题一致（`false`）
4. 在父容器的 `updateTheme()` 中传播主题到该控件
5. 确保 `loadData()` 等重建操作后调用 `updateTheme()` 同步主题

### 新增一个后台任务

1. 继承 `QThread`，重写 `run()`
2. 定义 `signals` 用于进度/结果回传（禁止在 run() 中操作 UI）
3. 在调用方（UI 层）connect 信号，用 `Qt::QueuedConnection`
4. 提供 `requestCancel()` 方法和 `m_cancelRequested` 标志

### 新增一个数据模型

1. 在 `src/models/` 新建头文件（仅 `.h`，无 `.cpp`）
2. 使用 `struct` + 辅助方法（纯函数，无信号槽）
3. 不继承 `QObject`，不包含 `<QWidget>` 相关头

---

## 构建与验证

```bat
# 全量构建
build.bat

# 仅运行 qmake 检查语法
cd build\release
qmake ..\..\WSLTool.pro -spec win32-g++ CONFIG+=release

# 检查编译警告（推荐无警告提交）
mingw32-make -j4 2>&1 | findstr /i "warning"
```

**代码提交前检查清单：**

- [ ] `build.bat` 可以无错误完成
- [ ] 无新增编译器警告（`-Wall`）
- [ ] 新增的 UI 组件已在 **两个** QSS 文件（dark + light）中添加样式
- [ ] 长耗时操作未在主线程调用
- [ ] `WSLTool.pro` 已同步更新新文件路径
- [ ] 自定义绘制控件的主题默认值与全局一致
- [ ] `loadData()` 中重建的控件已同步当前主题

---

## 文件权限与注意事项

- 迁移操作涉及注销 WSL 发行版，**不可逆**，务必在迁移向导中提示用户备份
- 读取注册表 `HKCU` 一般无需额外权限，但写入或访问 `HKLM` 需要管理员
- `wsl --export` 的临时 tar 文件默认放在目标目录，迁移完成后由 `stepCleanup()` 删除
- Inno Setup 打包时最低 Windows 版本为 6.1.7600（Windows 7 RTM）
- 系统托盘在 Windows 上需要 `QT += widgets`（已包含），无需额外库
- 单例检测使用 `QSharedMemory` + `QLocalServer`，需要 `QT += network`
- `CONFIG += windows` 而非 `console`：移除控制台窗口（生产构建），调试时可临时改为 `console`

---

## 相关资源

- [Qt 5.15 官方文档](https://doc.qt.io/qt-5/)
- [WSL 官方文档](https://learn.microsoft.com/zh-cn/windows/wsl/)
- [Inno Setup 文档](https://jrsoftware.org/isinfo.php)
- [WinAPI 参考](https://learn.microsoft.com/zh-cn/windows/win32/api/)
