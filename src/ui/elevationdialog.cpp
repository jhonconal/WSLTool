#include "elevationdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QSettings>
#include <QStyle>

ElevationDialog::ElevationDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("需要管理员权限 — WSL 管理工具");
    setModal(true);
    setFixedSize(480, 280);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUi();
}

void ElevationDialog::setupUi()
{
    QSettings settings("WSLTool", "Preferences");
    bool isDark = settings.value("darkTheme", true).toBool();

    QString bgColor       = isDark ? "#282c34" : "#ffffff";
    QString textColor     = isDark ? "#e2e8f0" : "#1f2937";
    QString descColor     = isDark ? "#94a3b8" : "#475569";
    QString accentColor   = isDark ? "#00d4aa" : "#008f72";
    QString dividerColor  = isDark ? "rgba(255,255,255,0.08)" : "rgba(0,0,0,0.08)";
    QString btnTextColor  = isDark ? "#141720" : "#ffffff";
    QString accentHover   = isDark ? "#00c49a" : "#007a63";

    setStyleSheet(QString(R"(
        ElevationDialog {
            background: %1;
        }
        #elevTitle {
            color: %2;
            font-size: 16px;
            font-weight: 700;
        }
        #elevDesc {
            color: %3;
            font-size: 13px;
        }
        #elevDivider {
            background: %5;
            max-height: 1px;
        }
        #elevBtnPrimary {
            background: %4;
            border: none;
            border-radius: 8px;
            color: %6;
            font-size: 14px;
            font-weight: 600;
            padding: 10px 28px;
        }
        #elevBtnPrimary:hover {
            background: %7;
        }
        #elevBtnSecondary {
            background: transparent;
            border: 1px solid %5;
            border-radius: 8px;
            color: %3;
            font-size: 13px;
            padding: 10px 24px;
        }
        #elevBtnSecondary:hover {
            background: rgba(128,128,128,0.08);
        }
    )").arg(bgColor, textColor, descColor, accentColor, dividerColor,
            btnTextColor, accentHover));

    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->setContentsMargins(32, 28, 32, 24);
    vl->setSpacing(16);

    // 图标 + 标题行
    QHBoxLayout *titleRow = new QHBoxLayout;
    titleRow->setSpacing(12);

    QLabel *icon = new QLabel;
    QPixmap shieldPix = style()->standardPixmap(QStyle::SP_MessageBoxWarning);
    if (!shieldPix.isNull()) {
        icon->setPixmap(shieldPix.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    titleRow->addWidget(icon);

    QLabel *title = new QLabel("需要管理员权限");
    title->setObjectName("elevTitle");
    titleRow->addWidget(title, 1);
    vl->addLayout(titleRow);

    // 分隔线
    QFrame *divider = new QFrame;
    divider->setObjectName("elevDivider");
    divider->setFrameShape(QFrame::HLine);
    vl->addWidget(divider);

    // 说明文字
    QLabel *desc = new QLabel(
        QString("WSL 管理工具需要<b style=\"color:%1;\">管理员权限</b>才能正常运行，"
                "用于执行以下操作："
                "<ul style=\"margin: 6px 0; padding-left: 20px;\">"
                "<li>读取 Windows 注册表（WSL 配置信息）</li>"
                "<li>执行 WSL 发行版的导入 / 导出 / 迁移</li>"
                "<li>管理虚拟磁盘文件（VHDX）</li>"
                "</ul>"
                "点击下方按钮以管理员身份重新启动程序。").arg(textColor));
    desc->setObjectName("elevDesc");
    desc->setWordWrap(true);
    desc->setTextFormat(Qt::RichText);
    vl->addWidget(desc);

    vl->addStretch();

    // 按钮行
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->setSpacing(12);

    QPushButton *exitBtn = new QPushButton("退出程序");
    exitBtn->setObjectName("elevBtnSecondary");
    exitBtn->setCursor(Qt::PointingHandCursor);

    QPushButton *elevateBtn = new QPushButton("以管理员身份重启");
    elevateBtn->setObjectName("elevBtnPrimary");
    elevateBtn->setCursor(Qt::PointingHandCursor);
    elevateBtn->setDefault(true);

    connect(exitBtn, &QPushButton::clicked, this, [this]() {
        m_shouldElevate = false;
        reject();
    });
    connect(elevateBtn, &QPushButton::clicked, this, [this]() {
        m_shouldElevate = true;
        accept();
    });

    btnRow->addWidget(exitBtn);
    btnRow->addStretch();
    btnRow->addWidget(elevateBtn);
    vl->addLayout(btnRow);
}
