#include "installdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QFrame>
#include <QMessageBox>
#include <QStyle>
#include <QDir>

InstallDialog::InstallDialog(const OnlineDistribution &distro, const QList<DiskInfo> &disks, QWidget *parent)
    : QDialog(parent), m_distro(distro), m_disks(disks)
{
    setWindowTitle("安装发行版 — " + distro.friendlyName);
    setModal(true);
    setMinimumWidth(520);
    setupUi();
}

void InstallDialog::setupUi()
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->setContentsMargins(28, 28, 28, 24);
    vl->setSpacing(20);

    // ---- 标题 ----
    QHBoxLayout *titleRow = new QHBoxLayout;
    QLabel *icon = new QLabel;
    QPixmap titlePix(":/icons/card_windows.svg");
    if (!titlePix.isNull()) {
        icon->setPixmap(titlePix.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    QVBoxLayout *titleVl = new QVBoxLayout;
    QLabel *title = new QLabel("安装 " + m_distro.friendlyName);
    title->setObjectName("dialogTitle");
    QLabel *sub = new QLabel("在线发行版一键下载与部署");
    sub->setObjectName("dialogSub");
    titleVl->addWidget(title);
    titleVl->addWidget(sub);
    titleRow->addWidget(icon);
    titleRow->addLayout(titleVl, 1);
    vl->addLayout(titleRow);

    // ---- 分隔线 ----
    auto mkSep = [&]() {
        QFrame *f = new QFrame;
        f->setFrameShape(QFrame::HLine);
        f->setObjectName("dialogSep");
        return f;
    };
    vl->addWidget(mkSep());

    // ---- 安装模式选择 ----
    QLabel *sec1 = new QLabel("安装方式");
    sec1->setObjectName("sectionHeader");
    vl->addWidget(sec1);

    m_defaultRadio = new QRadioButton("  默认安装 (安装到系统 C 盘默认路径)");
    m_defaultRadio->setObjectName("dialogRadio");
    m_defaultRadio->setChecked(true);
    vl->addWidget(m_defaultRadio);

    m_customRadio = new QRadioButton("  指定路径安装 (安装到自定义盘符/目录)");
    m_customRadio->setObjectName("dialogRadio");
    vl->addWidget(m_customRadio);

    // ---- 自定义路径配置区域 ----
    m_customSection = new QWidget;
    m_customSection->setVisible(false);
    QVBoxLayout *cl = new QVBoxLayout(m_customSection);
    cl->setContentsMargins(24, 0, 0, 0); // 缩进
    cl->setSpacing(10);

    QHBoxLayout *pathRow = new QHBoxLayout;
    pathRow->setSpacing(8);
    m_pathEdit = new QLineEdit;
    m_pathEdit->setPlaceholderText("请选择安装路径...");
    m_browseBtn = new QPushButton("浏览");
    m_browseBtn->setFixedWidth(64);
    m_browseBtn->setObjectName("dialogBrowseBtn");
    m_browseBtn->setCursor(Qt::PointingHandCursor);
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(m_browseBtn);
    cl->addLayout(pathRow);

    // 默认查找非 C 盘，若无则使用 C 盘
    QString defaultLetter = "C";
    for (const DiskInfo &d : m_disks) {
        if (d.letter.toUpper() != "C") {
            defaultLetter = d.letter;
            break;
        }
    }
    m_pathEdit->setText(defaultLetter + ":\\WSL\\" + m_distro.name);

    vl->addWidget(m_customSection);

    // ---- 空间提示与警告 ----
    QWidget *warnCard = new QWidget;
    warnCard->setObjectName("warnCard");
    QHBoxLayout *whl = new QHBoxLayout(warnCard);
    whl->setContentsMargins(14, 10, 14, 10);
    QLabel *warnIcon = new QLabel();
    warnIcon->setPixmap(QPixmap(":/icons/card_warning.svg").scaled(16, 16));
    QLabel *warnLbl = new QLabel(
        "<b>安装说明：</b><br>"
        "1. 安装将打开<b>外部命令行窗口</b>执行下载与系统注册，请在弹出的窗口中耐心等待。<br>"
        "2. 下载完成后，请根据控制台提示<b>配置您的用户名和密码</b>。<br>"
        "3. 若您选择指定路径，工具会在命令行安装完成后，<b>自动运行迁移任务</b>将发行版移至目标位置。");
    warnLbl->setObjectName("warnCardText");
    warnLbl->setWordWrap(true);
    whl->addWidget(warnIcon);
    whl->addWidget(warnLbl, 1);
    vl->addWidget(warnCard);

    vl->addWidget(mkSep());

    // ---- 底部按钮 ----
    QHBoxLayout *btnRow = new QHBoxLayout;
    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setFixedHeight(38);
    cancelBtn->setObjectName("dialogCancelBtn");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_startBtn = new QPushButton("开始安装");
    m_startBtn->setFixedHeight(38);
    m_startBtn->setObjectName("dialogStartBtn");
    m_startBtn->setCursor(Qt::PointingHandCursor);
    connect(m_startBtn, &QPushButton::clicked, this, &InstallDialog::onStartInstall);

    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(m_startBtn);
    vl->addLayout(btnRow);

    // 信号连接
    connect(m_defaultRadio, &QRadioButton::toggled, this, &InstallDialog::onInstallTypeChanged);
    connect(m_customRadio,  &QRadioButton::toggled, this, &InstallDialog::onInstallTypeChanged);
    connect(m_browseBtn,     &QPushButton::clicked, this, &InstallDialog::onBrowse);
    connect(m_pathEdit,      &QLineEdit::textChanged, this, &InstallDialog::validateForm);

    validateForm();
}

void InstallDialog::onInstallTypeChanged()
{
    bool isCustom = m_customRadio->isChecked();
    m_customSection->setVisible(isCustom);
    adjustSize();
    validateForm();
}

void InstallDialog::onBrowse()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "选择目标目录", m_pathEdit->text().isEmpty() ? "C:\\" : m_pathEdit->text());
    if (!dir.isEmpty()) {
        m_pathEdit->setText(QDir::toNativeSeparators(dir));
        validateForm();
    }
}

void InstallDialog::validateForm()
{
    bool ok = true;
    if (m_customRadio->isChecked()) {
        ok = !m_pathEdit->text().trimmed().isEmpty();
    }
    m_startBtn->setEnabled(ok);
}

bool InstallDialog::isDefaultInstall() const
{
    return m_defaultRadio->isChecked();
}

QString InstallDialog::targetPath() const
{
    if (isDefaultInstall()) return "";
    return QDir::toNativeSeparators(m_pathEdit->text().trimmed());
}

void InstallDialog::onStartInstall()
{
    accept();
}
