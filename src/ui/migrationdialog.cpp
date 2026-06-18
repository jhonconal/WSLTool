#include "migrationdialog.h"
#include "migrationprogressdialog.h"
#include "../models/diskinfo.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QVariant>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QFrame>
#include <QMessageBox>

MigrationDialog::MigrationDialog(const WslDistribution &distro,
                                  const QList<DiskInfo> &disks,
                                  QWidget *parent)
    : QDialog(parent), m_distro(distro), m_disks(disks)
{
    setWindowTitle("迁移发行版 — " + distro.displayName);
    setModal(true);
    setMinimumWidth(520);

    setupUi();
}

void MigrationDialog::setupUi()
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->setContentsMargins(28, 28, 28, 24);
    vl->setSpacing(20);

    // ---- 标题 ----
    QHBoxLayout *titleRow = new QHBoxLayout;
    QLabel *icon = new QLabel;
    QPixmap titlePix(":/icons/card_disk.svg");
    if (!titlePix.isNull()) {
        icon->setPixmap(titlePix.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    QVBoxLayout *titleVl = new QVBoxLayout;
    QLabel *title = new QLabel("迁移 " + m_distro.displayName);
    title->setObjectName("dialogTitle");
    QLabel *sub = new QLabel(QString("当前位置: %1  (%2)")
                                 .arg(m_distro.diskLetter + "盘",
                                      m_distro.formattedSize()));
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

    // ---- 目标配置 ----
    QLabel *sec1 = new QLabel("目标位置");
    sec1->setObjectName("sectionHeader");
    vl->addWidget(sec1);

    QGridLayout *grid = new QGridLayout;
    grid->setSpacing(10);
    grid->setColumnStretch(1, 1);

    // 目标磁盘
    QLabel *diskLbl = new QLabel("目标磁盘");
    diskLbl->setObjectName("dialogLabel");
    m_diskCombo = new QComboBox;
    for (const DiskInfo &d : m_disks) {
        if (d.letter.toUpper() == "C") continue; // 排除 C 盘
        QString item = QString("%1:\\  (%2 可用)").arg(d.letter, d.formattedFree());
        m_diskCombo->addItem(item, d.letter);
    }
    if (m_diskCombo->count() == 0) {
        m_diskCombo->addItem("无其他磁盘", "");
    }
    grid->addWidget(diskLbl,     0, 0);
    grid->addWidget(m_diskCombo, 0, 1);

    // 目标路径
    QLabel *pathLbl = new QLabel("目标路径");
    pathLbl->setObjectName("dialogLabel");
    m_pathEdit = new QLineEdit;
    m_pathEdit->setPlaceholderText("选择磁盘后自动填充，或手动输入...");
    QPushButton *browseBtn = new QPushButton("浏览");
    browseBtn->setFixedWidth(64);
    browseBtn->setObjectName("dialogBrowseBtn");

    QHBoxLayout *pathRow = new QHBoxLayout;
    pathRow->setSpacing(8);
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(browseBtn);
    grid->addWidget(pathLbl, 1, 0);
    grid->addLayout(pathRow, 1, 1);
    vl->addLayout(grid);

    // 空间检测
    QWidget *spaceCard = new QWidget;
    spaceCard->setObjectName("spaceCard");
    QHBoxLayout *shl = new QHBoxLayout(spaceCard);
    shl->setContentsMargins(14, 10, 14, 10);
    m_sizeLabel = new QLabel(QString("当前大小: %1").arg(m_distro.formattedSize()));
    m_sizeLabel->setObjectName("spaceCheckLabel");
    m_spaceLabel = new QLabel("请选择目标磁盘");
    m_spaceLabel->setObjectName("spaceStatusLabel");
    m_spaceLabel->setProperty("status", "normal");
    shl->addWidget(m_sizeLabel, 1);
    shl->addWidget(m_spaceLabel);
    vl->addWidget(spaceCard);

    // ---- 警告 ----
    QWidget *warnCard = new QWidget;
    warnCard->setObjectName("warnCard");
    QHBoxLayout *whl = new QHBoxLayout(warnCard);
    whl->setContentsMargins(14, 10, 14, 10);
    QLabel *warnIcon = new QLabel();
    warnIcon->setPixmap(QPixmap(":/icons/card_warning.svg").scaled(16, 16));
    QLabel *warnLbl = new QLabel(
        "<b>迁移前请做好备份！</b>迁移过程中请勿关闭窗口或强制中断，"
        "否则可能导致发行版损坏（工具会自动尝试回滚）。"
        "预计耗时 <b>3~20 分钟</b>，取决于发行版大小和磁盘速度。");
    warnLbl->setObjectName("warnCardText");
    warnLbl->setWordWrap(true);
    whl->addWidget(warnIcon);
    whl->addWidget(warnLbl);
    vl->addWidget(warnCard);

    vl->addWidget(mkSep());

    // ---- 账户配置 ----
    m_configAccountCheck = new QCheckBox("  迁移后配置默认账户和密码（可选）");
    m_configAccountCheck->setObjectName("dialogCheckBox");
    vl->addWidget(m_configAccountCheck);

    m_accountSection = new QWidget;
    m_accountSection->setVisible(false);
    QGridLayout *ag = new QGridLayout(m_accountSection);
    ag->setSpacing(10);
    ag->setColumnStretch(1, 1);
    ag->setContentsMargins(0, 0, 0, 0);

    auto mkLbl = [](const QString &t) {
        QLabel *l = new QLabel(t);
        l->setObjectName("dialogLabel");
        return l;
    };

    m_userEdit = new QLineEdit;
    m_userEdit->setPlaceholderText("默认用户名，e.g. ubuntu");
    m_passEdit = new QLineEdit;
    m_passEdit->setPlaceholderText("新密码");
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_pass2Edit = new QLineEdit;
    m_pass2Edit->setPlaceholderText("确认密码");
    m_pass2Edit->setEchoMode(QLineEdit::Password);

    ag->addWidget(mkLbl("默认用户"),  0, 0);
    ag->addWidget(m_userEdit,          0, 1);
    ag->addWidget(mkLbl("新密码"),    1, 0);
    ag->addWidget(m_passEdit,          1, 1);
    ag->addWidget(mkLbl("确认密码"),  2, 0);
    ag->addWidget(m_pass2Edit,         2, 1);
    vl->addWidget(m_accountSection);

    vl->addWidget(mkSep());

    // ---- 底部按钮 ----
    QHBoxLayout *btnRow = new QHBoxLayout;
    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setFixedHeight(38);
    cancelBtn->setObjectName("dialogCancelBtn");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_startBtn = new QPushButton("开始迁移");
    m_startBtn->setFixedHeight(38);
    m_startBtn->setObjectName("dialogStartBtn");
    connect(m_startBtn, &QPushButton::clicked, this, &MigrationDialog::onStartMigration);

    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(m_startBtn);
    vl->addLayout(btnRow);

    // 信号连接
    connect(m_diskCombo,         QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MigrationDialog::onDiskChanged);
    connect(m_pathEdit,          &QLineEdit::textChanged,
            this, &MigrationDialog::validateForm);
    connect(browseBtn,           &QPushButton::clicked,
            this, &MigrationDialog::onBrowse);
    connect(m_configAccountCheck, &QCheckBox::toggled, this, [this](bool on) {
        m_accountSection->setVisible(on);
        adjustSize();
    });

    // 初始化
    onDiskChanged(0);
    validateForm();
}

void MigrationDialog::onDiskChanged(int index)
{
    if (index < 0 || m_diskCombo->count() == 0) return;
    QString letter = m_diskCombo->itemData(index).toString();
    if (!letter.isEmpty()) {
        m_pathEdit->setText(letter + ":\\WSL\\" + m_distro.name);
    }
    updateSpaceCheck();
    validateForm();
}

void MigrationDialog::onBrowse()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "选择目标目录", m_pathEdit->text().isEmpty() ? "C:\\" : m_pathEdit->text());
    if (!dir.isEmpty()) {
        m_pathEdit->setText(QDir::toNativeSeparators(dir));
        updateSpaceCheck();
        validateForm();
    }
}

void MigrationDialog::updateSpaceCheck()
{
    QString letter = m_diskCombo->itemData(m_diskCombo->currentIndex()).toString();
    if (letter.isEmpty()) {
        m_spaceLabel->setText("❓ 无法检测");
        return;
    }

    // 在已知磁盘中找
    for (const DiskInfo &d : m_disks) {
        if (d.letter.toUpper() == letter.toUpper()) {
            bool enough = d.hasEnoughSpace(m_distro.sizeBytes > 0 ? m_distro.sizeBytes : 0);
            if (enough) {
                m_spaceLabel->setText("空间充足  (" + d.formattedFree() + " 可用)");
                m_spaceLabel->setProperty("status", "ok");
            } else {
                m_spaceLabel->setText("空间不足！需要 " + DiskInfo::formatBytes((qint64)(m_distro.sizeBytes * 1.25))
                                      + "，可用 " + d.formattedFree());
                m_spaceLabel->setProperty("status", "error");
            }
            m_spaceLabel->style()->unpolish(m_spaceLabel);
            m_spaceLabel->style()->polish(m_spaceLabel);
            m_spaceLabel->update();
            return;
        }
    }
    m_spaceLabel->setText("无法获取磁盘信息");
    m_spaceLabel->setProperty("status", "normal");
    m_spaceLabel->style()->unpolish(m_spaceLabel);
    m_spaceLabel->style()->polish(m_spaceLabel);
    m_spaceLabel->update();
}

void MigrationDialog::validateForm()
{
    bool ok = !m_pathEdit->text().trimmed().isEmpty();

    // 检查账户配置完整性
    if (m_configAccountCheck->isChecked()) {
        if (!m_userEdit->text().trimmed().isEmpty()) {
            if (!m_passEdit->text().isEmpty() &&
                m_passEdit->text() != m_pass2Edit->text()) {
                ok = false;
            }
        }
    }

    m_startBtn->setEnabled(ok);
}

void MigrationDialog::onStartMigration()
{
    // 二次确认
    QString confirmMsg = QString(
        "确认将 <b>%1</b> 迁移到:<br><br>"
        "<code>%2</code><br><br>"
        "请确保已备份重要数据！迁移过程中请勿关闭程序。<br>"
        "如果迁移失败，工具将自动尝试回滚。<br><br>"
        "是否继续？")
        .arg(m_distro.displayName, m_pathEdit->text());

    QMessageBox mb(QMessageBox::Warning, "确认迁移", confirmMsg,
                   QMessageBox::Yes | QMessageBox::No, this);
    mb.button(QMessageBox::Yes)->setText("确认迁移");
    mb.button(QMessageBox::No)->setText("取消");

    if (mb.exec() != QMessageBox::Yes) return;

    MigrationConfig config;
    config.distro           = m_distro;
    config.targetDirectory  = QDir::toNativeSeparators(m_pathEdit->text().trimmed());
    config.configureAccount = m_configAccountCheck->isChecked();
    config.newUsername      = m_userEdit->text().trimmed();
    config.newPassword      = m_passEdit->text();

    accept();

    MigrationProgressDialog *prog = new MigrationProgressDialog(config, parentWidget());
    prog->setAttribute(Qt::WA_DeleteOnClose);
    prog->exec();
}

