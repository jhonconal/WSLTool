#include "migrationprogressdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QStyle>
#include <QVariant>
#include <QTextEdit>
#include <QPushButton>
#include <QCloseEvent>
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>

MigrationProgressDialog::MigrationProgressDialog(const MigrationConfig &config,
                                                   QWidget *parent)
    : QDialog(parent)
    , m_config(config)
    , m_worker(nullptr)
    , m_finished(false)
{
    setWindowTitle("迁移进度 — " + config.distro.displayName);
    setModal(true);
    setMinimumSize(580, 520);
    setupUi();

    // 启动工作线程
    m_worker = new MigrationWorker(m_config, this);
    connect(m_worker, &MigrationWorker::stepChanged,   this, &MigrationProgressDialog::onStepChanged);
    connect(m_worker, &MigrationWorker::logMessage,    this, &MigrationProgressDialog::onLogMessage);
    connect(m_worker, &MigrationWorker::progressValue, this, &MigrationProgressDialog::onProgressValue);
    connect(m_worker, &MigrationWorker::finished,      this, &MigrationProgressDialog::onFinished);
    m_worker->start();
}

MigrationProgressDialog::~MigrationProgressDialog()
{
    if (m_worker && m_worker->isRunning()) {
        m_worker->requestCancel();
        m_worker->wait(10000);
    }
}

void MigrationProgressDialog::setupUi()
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->setContentsMargins(28, 24, 28, 20);
    vl->setSpacing(16);

    // 标题
    QLabel *title = new QLabel("正在迁移: " + m_config.distro.displayName);
    title->setObjectName("dialogTitle");
    vl->addWidget(title);

    // 步骤指示器
    m_stepArea = new QWidget;
    m_stepArea->setObjectName("progressStepArea");
    QHBoxLayout *stepLyt = new QHBoxLayout(m_stepArea);
    stepLyt->setContentsMargins(0, 0, 0, 0);
    stepLyt->setSpacing(0);
    vl->addWidget(m_stepArea);

    // 当前步骤描述
    m_stepLabel = new QLabel("正在初始化...");
    m_stepLabel->setObjectName("progressStepLabel");
    m_stepLabel->setProperty("status", "normal");
    vl->addWidget(m_stepLabel);

    // 进度条
    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(14);
    vl->addWidget(m_progressBar);

    // 实时日志
    QLabel *logTitle = new QLabel("详细日志");
    logTitle->setObjectName("dialogLabel");
    vl->addWidget(logTitle);

    m_logEdit = new QTextEdit;
    m_logEdit->setObjectName("progressLogEdit");
    m_logEdit->setReadOnly(true);
    m_logEdit->setMinimumHeight(220);
    vl->addWidget(m_logEdit, 1);

    // 状态标签
    m_statusLabel = new QLabel("运行中...");
    m_statusLabel->setObjectName("progressStatusLabel");
    m_statusLabel->setProperty("status", "normal");
    vl->addWidget(m_statusLabel);

    // 取消按钮
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    m_cancelBtn = new QPushButton("中止迁移");
    m_cancelBtn->setFixedHeight(36);
    m_cancelBtn->setObjectName("progressCancelBtn");
    connect(m_cancelBtn, &QPushButton::clicked, this, &MigrationProgressDialog::onCancel);
    btnRow->addWidget(m_cancelBtn);
    vl->addLayout(btnRow);
}

void MigrationProgressDialog::buildStepIndicator(int total)
{
    QHBoxLayout *lyt = static_cast<QHBoxLayout*>(m_stepArea->layout());
    QLayoutItem *item;
    while ((item = lyt->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_stepDots.clear();
    m_stepLabels.clear();

    QStringList labels;
    if (total == 6)
        labels << "停止" << "导出" << "注销" << "导入" << "账户" << "清理";
    else
        labels << "步骤1" << "步骤2" << "步骤3" << "步骤4" << "步骤5" << "步骤6";

    for (int i = 0; i < total; i++) {
        QWidget *col = new QWidget;
        col->setObjectName("progressStepCol");
        QVBoxLayout *vl = new QVBoxLayout(col);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->setSpacing(2);

        QLabel *dot = new QLabel("○");
        dot->setAlignment(Qt::AlignCenter);
        dot->setObjectName("stepDot");
        dot->setProperty("status", "waiting");
        dot->setFixedWidth(32);
        m_stepDots.append(dot);

        QLabel *lbl = new QLabel(i < labels.size() ? labels[i] : "");
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setObjectName("stepLabel");
        lbl->setProperty("status", "waiting");
        m_stepLabels.append(lbl);

        vl->addWidget(dot);
        vl->addWidget(lbl);
        lyt->addWidget(col);

        if (i < total - 1) {
            QLabel *line = new QLabel("────");
            line->setAlignment(Qt::AlignCenter);
            line->setObjectName("stepLine");
            lyt->addWidget(line);
        }
    }
}

void MigrationProgressDialog::setStepActive(int step)
{
    for (int i = 0; i < m_stepDots.size(); i++) {
        if (i < step - 1) {
            m_stepDots[i]->setText("●");
            m_stepDots[i]->setProperty("status", "completed");
            m_stepLabels[i]->setProperty("status", "completed");
        } else if (i == step - 1) {
            m_stepDots[i]->setText("◉");
            m_stepDots[i]->setProperty("status", "active");
            m_stepLabels[i]->setProperty("status", "active");
        } else {
            m_stepDots[i]->setText("○");
            m_stepDots[i]->setProperty("status", "waiting");
            m_stepLabels[i]->setProperty("status", "waiting");
        }
        m_stepDots[i]->style()->unpolish(m_stepDots[i]);
        m_stepDots[i]->style()->polish(m_stepDots[i]);
        m_stepDots[i]->update();

        m_stepLabels[i]->style()->unpolish(m_stepLabels[i]);
        m_stepLabels[i]->style()->polish(m_stepLabels[i]);
        m_stepLabels[i]->update();
    }
}

void MigrationProgressDialog::onStepChanged(int step, int total, const QString &desc)
{
    if (m_stepDots.isEmpty()) {
        buildStepIndicator(total);
    }
    setStepActive(step);
    m_stepLabel->setText(QString("[步骤 %1/%2] %3").arg(step).arg(total).arg(desc));
}

void MigrationProgressDialog::onLogMessage(const QString &msg)
{
    QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logEdit->append("[" + ts + "] " + msg);
    m_logEdit->verticalScrollBar()->setValue(m_logEdit->verticalScrollBar()->maximum());
}

void MigrationProgressDialog::onProgressValue(int percent)
{
    m_progressBar->setValue(percent);
}

void MigrationProgressDialog::onFinished(bool success, const QString &message)
{
    m_finished = true;
    m_cancelBtn->setEnabled(false);
    m_cancelBtn->setText("已完成");

    if (success) {
        // 所有步骤变绿
        for (int i = 0; i < m_stepDots.size(); i++) {
            m_stepDots[i]->setText("●");
            m_stepDots[i]->setProperty("status", "completed");
            m_stepDots[i]->style()->unpolish(m_stepDots[i]);
            m_stepDots[i]->style()->polish(m_stepDots[i]);
            m_stepDots[i]->update();

            m_stepLabels[i]->setProperty("status", "completed");
            m_stepLabels[i]->style()->unpolish(m_stepLabels[i]);
            m_stepLabels[i]->style()->polish(m_stepLabels[i]);
            m_stepLabels[i]->update();
        }
        m_stepLabel->setText("迁移成功完成！");
        m_stepLabel->setProperty("status", "ok");
        m_statusLabel->setText(message);
        m_statusLabel->setProperty("status", "ok");

        m_stepLabel->style()->unpolish(m_stepLabel);
        m_stepLabel->style()->polish(m_stepLabel);
        m_stepLabel->update();
        m_statusLabel->style()->unpolish(m_statusLabel);
        m_statusLabel->style()->polish(m_statusLabel);
        m_statusLabel->update();

        QMessageBox mb(QMessageBox::Information, "迁移成功",
                       "<b>" + m_config.distro.displayName + "</b> 迁移成功！<br><br>" + message,
                       QMessageBox::Ok, this);
        mb.exec();
    } else {
        for (int i = 0; i < m_stepDots.size(); i++) {
            m_stepDots[i]->setProperty("status", "error");
            m_stepDots[i]->style()->unpolish(m_stepDots[i]);
            m_stepDots[i]->style()->polish(m_stepDots[i]);
            m_stepDots[i]->update();

            m_stepLabels[i]->setProperty("status", "error");
            m_stepLabels[i]->style()->unpolish(m_stepLabels[i]);
            m_stepLabels[i]->style()->polish(m_stepLabels[i]);
            m_stepLabels[i]->update();
        }
        m_stepLabel->setText("迁移失败");
        m_stepLabel->setProperty("status", "error");
        m_statusLabel->setText(message);
        m_statusLabel->setProperty("status", "error");

        m_stepLabel->style()->unpolish(m_stepLabel);
        m_stepLabel->style()->polish(m_stepLabel);
        m_stepLabel->update();
        m_statusLabel->style()->unpolish(m_statusLabel);
        m_statusLabel->style()->polish(m_statusLabel);
        m_statusLabel->update();

        QMessageBox mb(QMessageBox::Critical, "迁移失败",
                       "<b>" + m_config.distro.displayName + "</b> 迁移失败！<br><br>" + message,
                       QMessageBox::Ok, this);
        mb.exec();
    }

    QPushButton *closeBtn = new QPushButton("关闭");
    closeBtn->setStyleSheet(
        "background: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.1);"
        "border-radius: 8px; color: #e2e8f0; padding: 8px 24px; font-size: 13px;");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    static_cast<QVBoxLayout*>(layout())->addWidget(closeBtn, 0, Qt::AlignRight);
}

void MigrationProgressDialog::onCancel()
{
    QMessageBox mb(QMessageBox::Warning, "中止迁移",
                   "确认中止迁移？\n\n工具将尝试自动回滚，但可能造成发行版损坏。",
                   QMessageBox::Yes | QMessageBox::No, this);
    mb.button(QMessageBox::Yes)->setText("确认中止");
    mb.button(QMessageBox::No)->setText("继续迁移");
    mb.setStyleSheet("QMessageBox { background: #1a1d2e; color: #e2e8f0; }"
                     "QPushButton { background: #262a40; color: #e2e8f0; "
                     "border: 1px solid rgba(255,255,255,0.1); border-radius: 6px; padding: 6px 16px; }");

    if (mb.exec() == QMessageBox::Yes && m_worker) {
        m_worker->requestCancel();
        m_cancelBtn->setEnabled(false);
        m_statusLabel->setText("正在中止并回滚...");
    }
}

void MigrationProgressDialog::closeEvent(QCloseEvent *event)
{
    if (!m_finished && m_worker && m_worker->isRunning()) {
        QMessageBox mb(QMessageBox::Warning, "迁移进行中",
                       "迁移仍在进行中，强制关闭可能导致数据损坏！\n\n确认关闭？",
                       QMessageBox::Yes | QMessageBox::No, this);
        mb.button(QMessageBox::Yes)->setText("强制关闭");
        mb.button(QMessageBox::No)->setText("返回等待");
        if (mb.exec() != QMessageBox::Yes) {
            event->ignore();
            return;
        }
        m_worker->requestCancel();
        m_worker->wait(5000);
    }
    QDialog::closeEvent(event);
}

