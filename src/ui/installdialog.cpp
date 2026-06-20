#include "installdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QFrame>
#include <QMessageBox>
#include <QStyle>
#include <QDir>
#include <QCheckBox>

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
    vl->addWidget(mkSep());

    // ---- 账号配置 ----
    m_configAccountCheck = new QCheckBox("  自动配置系统默认登录账户（推荐）");
    m_configAccountCheck->setObjectName("dialogCheckBox");
    m_configAccountCheck->setChecked(true);
    vl->addWidget(m_configAccountCheck);

    m_accountSection = new QWidget;
    QGridLayout *ag = new QGridLayout(m_accountSection);
    ag->setSpacing(10);
    ag->setColumnStretch(1, 1);
    ag->setContentsMargins(24, 0, 0, 0); // 缩进

    auto mkLbl = [](const QString &t) {
        QLabel *l = new QLabel(t);
        l->setObjectName("dialogLabel");
        return l;
    };

    m_userEdit = new QLineEdit;
    m_userEdit->setPlaceholderText("默认用户名，e.g. wsluser");
    m_userEdit->setText("wsluser");
    
    m_passEdit = new QLineEdit;
    m_passEdit->setPlaceholderText("系统登录密码");
    m_passEdit->setEchoMode(QLineEdit::Password);
    
    m_pass2Edit = new QLineEdit;
    m_pass2Edit->setPlaceholderText("确认登录密码");
    m_pass2Edit->setEchoMode(QLineEdit::Password);

    ag->addWidget(mkLbl("用户名"),   0, 0);
    ag->addWidget(m_userEdit,      0, 1);
    ag->addWidget(mkLbl("设置密码"), 1, 0);
    ag->addWidget(m_passEdit,      1, 1);
    ag->addWidget(mkLbl("确认密码"), 2, 0);
    ag->addWidget(m_pass2Edit,     2, 1);
    vl->addWidget(m_accountSection);

    vl->addWidget(mkSep());

    // ---- 空间提示与警告 ----
    QWidget *warnCard = new QWidget;
    warnCard->setObjectName("warnCard");
    QHBoxLayout *whl = new QHBoxLayout(warnCard);
    whl->setContentsMargins(14, 10, 14, 10);
    QLabel *warnIcon = new QLabel();
    warnIcon->setPixmap(QPixmap(":/icons/card_warning.svg").scaled(16, 16));
    QLabel *warnLbl = new QLabel(
        "<b>安装部署说明：</b><br>"
        "1. 系统将自动在后台下载包、注册发行版、静默配置账户，无需您手动在终端交互输入！<br>"
        "2. 下载部署可能需要约 <b>2~15 分钟</b>，取决于发行版文件大小及网络速度。<br>"
        "3. 若选择了自定义路径，安装注册成功后，工具会自动将发行版<b>迁移并重新挂载</b>至您的目标位置。");
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
    connect(m_configAccountCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_accountSection->setVisible(checked);
        adjustSize();
        validateForm();
    });
    connect(m_userEdit,  &QLineEdit::textChanged, this, &InstallDialog::validateForm);
    connect(m_passEdit,  &QLineEdit::textChanged, this, &InstallDialog::validateForm);
    connect(m_pass2Edit, &QLineEdit::textChanged, this, &InstallDialog::validateForm);

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
    if (m_configAccountCheck->isChecked()) {
        if (m_userEdit->text().trimmed().isEmpty()) {
            ok = false;
        } else if (!m_passEdit->text().isEmpty() && m_passEdit->text() != m_pass2Edit->text()) {
            ok = false;
        }
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

bool InstallDialog::createAccount() const
{
    return m_configAccountCheck->isChecked();
}

QString InstallDialog::username() const
{
    return m_userEdit->text().trimmed();
}

QString InstallDialog::password() const
{
    return m_passEdit->text();
}

void InstallDialog::onStartInstall()
{
    accept();
}
