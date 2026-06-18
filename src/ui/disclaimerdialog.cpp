#include "disclaimerdialog.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSettings>

DisclaimerDialog::DisclaimerDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("免责声明 — WSL 管理工具");
    setModal(true);
    setFixedWidth(520);
    setupUi();
}

bool DisclaimerDialog::shouldShow()
{
    QSettings s("WSLTool", "Preferences");
    return !s.value("disclaimerAccepted", false).toBool();
}

void DisclaimerDialog::markAsShown()
{
    QSettings s("WSLTool", "Preferences");
    s.setValue("disclaimerAccepted", true);
}

void DisclaimerDialog::setupUi()
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->setContentsMargins(32, 28, 32, 24);
    vl->setSpacing(18);

    QSettings settings("WSLTool", "Preferences");
    bool isDark = settings.value("darkTheme", true).toBool();

    QString normalColor = isDark ? "#94a3b8" : "#475569";
    QString boldColor   = isDark ? "#e2e8f0" : "#1f2937";
    QString warnColor   = isDark ? "#ff4d6d" : "#dc2626";
    QString highColor   = isDark ? "#fbbf24" : "#d97706";

    // 图标 + 标题
    QHBoxLayout *titleRow = new QHBoxLayout;
    QLabel *icon = new QLabel;
    QPixmap warnPix(":/icons/card_warning.svg");
    if (!warnPix.isNull()) {
        icon->setPixmap(warnPix.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    QLabel *title = new QLabel("使用前请阅读免责声明");
    title->setObjectName("disclaimerTitle");
    titleRow->addWidget(icon);
    titleRow->addWidget(title, 1);
    vl->addLayout(titleRow);

    // 分隔线
    QFrame *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setObjectName("disclaimerSep");
    vl->addWidget(sep);

    // 声明内容
    QString disclaimerText = QString(R"(
<p style="color:%1; font-size:13px; line-height:1.7;">
<b style="color:%2;">WSL 管理工具</b> 是一款用于管理 Windows Subsystem for Linux（WSL）发行版的辅助工具，
仅供个人学习和使用。在使用本工具前，请仔细阅读以下注意事项：
</p>
<ol style="color:%1; font-size:13px; line-height:2; margin-left:16px;">
<li><b style="color:%4;">数据风险</b>：迁移操作涉及发行版的导出、注销和重新导入，
操作失败可能导致数据丢失。<b style="color:%3;">请务必在迁移前做好完整备份。</b></li>

<li><b style="color:%4;">免责申明</b>：因使用本工具进行的任何操作（包括但不限于迁移、
账户配置、注销等）而导致的任何直接或间接损失，<b style="color:%3;">概不负责。</b></li>

<li><b style="color:%4;">管理员权限</b>：本工具运行需要管理员权限，部分操作会修改
Windows 注册表和系统文件。请确认您了解相关风险。</li>

<li><b style="color:%4;">回滚机制</b>：迁移失败时工具会自动尝试回滚，但<b style="color:%3;">无法
保证回滚一定成功。</b>备份文件（.tar）将保留在目标目录供手动恢复。</li>

<li><b style="color:%4;">兼容性</b>：本工具适用于 Windows 10 1903 及以上版本，
在 Windows 7/8 上仅支持基础检测功能。</li>
</ol>
<p style="color:%1; font-size:11px; margin-top:8px;">
使用本工具即表示您已阅读、理解并同意上述条款。
</p>
)").arg(normalColor, boldColor, warnColor, highColor);

    QLabel *content = new QLabel(disclaimerText);
    content->setWordWrap(true);
    content->setTextFormat(Qt::RichText);
    content->setOpenExternalLinks(false);
    vl->addWidget(content);

    // "不再显示"
    QCheckBox *noShow = new QCheckBox("  我已阅读并理解上述风险，不再显示此声明");
    noShow->setObjectName("disclaimerNoShow");
    QCheckBox *noShow2 = noShow; // 供 lambda 捕获
    vl->addWidget(noShow);

    // 按钮
    QHBoxLayout *btnRow = new QHBoxLayout;
    QPushButton *rejectBtn = new QPushButton("拒绝并退出");
    rejectBtn->setFixedHeight(38);
    rejectBtn->setObjectName("disclaimerRejectBtn");

    QPushButton *acceptBtn = new QPushButton("我已了解风险，继续使用");
    acceptBtn->setFixedHeight(38);
    acceptBtn->setObjectName("disclaimerAcceptBtn");

    connect(rejectBtn, &QPushButton::clicked, this, [this]() {
        reject();
        qApp->quit();
    });
    connect(acceptBtn, &QPushButton::clicked, this, [this, noShow2]() {
        if (noShow2->isChecked()) markAsShown();
        accept();
    });

    btnRow->addWidget(rejectBtn);
    btnRow->addStretch();
    btnRow->addWidget(acceptBtn);
    vl->addLayout(btnRow);
}

