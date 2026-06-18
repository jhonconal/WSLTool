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
├── main.cpp                     # 入口：UAC 提权检测、QSS 主题、字体、DisclaimerDialog
├── mainwindow.h/cpp             # 主窗口：无边框拖拽、侧边栏路由、数据加载
├── core/                        # 纯业务逻辑，无 UI 依赖
│   ├── wslmanager               # 注册表枚举 WSL 发行版、调用 wsl.exe
│   ├── migrationworker          # 迁移任务（QThread 子类，分步执行）
│   ├── systemdetector           # Windows 版本、WSL 版本检测
│   └── diskmanager              # GetDiskFreeSpaceEx / WMI 磁盘信息
├── models/                      # 纯数据结构头文件，无 Qt 对象
│   ├── wsldistribution.h        # WslDistribution struct + WslVersion/DistroState enum
│   ├── diskinfo.h               # DiskInfo struct
│   └── systeminfo.h             # SystemInfo struct
└── ui/                          # Qt Widgets UI 层
    ├── dashboardpage            # 首页：SystemInfo + DiskInfo 展示
    ├── distributionpage         # 发行版列表：DistroCard 网格
    ├── migrationdialog          # 迁移向导：步骤 1 配置
    ├── migrationprogressdialog  # 迁移向导：步骤 2 实时进度
    ├── disclaimerdialog         # 首次运行免责声明（QSettings 持久化状态）
    └── widgets/
        ├── distrocard           # 单个发行版卡片（含迁移按钮）
        ├── diskusagebar         # 磁盘使用率可视化条
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

---

## 编码规范

### 命名约定

| 类别 | 风格 | 示例 |
|------|------|------|
| 类名 | PascalCase | `WslManager`, `DistroCard` |
| 成员变量 | `m_` 前缀 + camelCase | `m_distros`, `m_cancelRequested` |
| 私有方法 | camelCase | `stepStopDistro()`, `setupUi()` |
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

- 所有 UI 样式通过 `resources/styles/dark_theme.qss` 统一管理
- 不在 `.cpp` 中硬编码颜色字符串
- 新增 Widget 需在 QSS 中添加对应的 objectName 选择器

---

## 关键模块说明

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

---

## 添加新功能的指南

### 新增一个 UI 页面

1. 在 `src/ui/` 创建 `yourpage.h` 和 `yourpage.cpp`
2. 在 `WSLTool.pro` 的 `SOURCES` 和 `HEADERS` 中添加路径
3. 在 `mainwindow.h` 中前向声明并添加成员指针
4. 在 `MainWindow::setupSidebar()` 中添加 `SidebarButton`
5. 在 `MainWindow::setupContent()` 中添加到 `m_stack`
6. 在 `resources/styles/dark_theme.qss` 中添加样式

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
- [ ] 新增的 UI 组件已添加 QSS 样式
- [ ] 长耗时操作未在主线程调用
- [ ] `WSLTool.pro` 已同步更新新文件路径

---

## 文件权限与注意事项

- 迁移操作涉及注销 WSL 发行版，**不可逆**，务必在迁移向导中提示用户备份
- 读取注册表 `HKCU` 一般无需额外权限，但写入或访问 `HKLM` 需要管理员
- `wsl --export` 的临时 tar 文件默认放在目标目录，迁移完成后由 `stepCleanup()` 删除
- Inno Setup 打包时最低 Windows 版本为 6.1.7600（Windows 7 RTM）

---

## 相关资源

- [Qt 5.15 官方文档](https://doc.qt.io/qt-5/)
- [WSL 官方文档](https://learn.microsoft.com/zh-cn/windows/wsl/)
- [Inno Setup 文档](https://jrsoftware.org/isinfo.php)
- [WinAPI 参考](https://learn.microsoft.com/zh-cn/windows/win32/api/)
