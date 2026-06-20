#include "installprogressdialog.h"
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
#include <QKeyEvent>

InstallProgressDialog::InstallProgressDialog(const InstallConfig &config, QWidget *parent)
    : QDialog(parent)
    , m_config(config)
    , m_worker(nullptr)
    , m_finished(false)
{
    setWindowTitle("部署进度 — " + config.friendlyName);
    setModal(true);
    setMinimumSize(600, 560);
    setupUi();

    // 启动安装部署工作线程
    m_worker = new InstallWorker(m_config, this);
    connect(m_worker, &InstallWorker::stepChanged,   this, &InstallProgressDialog::onStepChanged);
    connect(m_worker, &InstallWorker::logMessage,    this, &InstallProgressDialog::onLogMessage);
    connect(m_worker, &InstallWorker::progressValue, this, &InstallProgressDialog::onProgressValue);
    connect(m_worker, &InstallWorker::finished,      this, &InstallProgressDialog::onFinished);
    m_worker->start();
}

InstallProgressDialog::~InstallProgressDialog()
{
    if (m_worker && m_worker->isRunning()) {
        m_worker->requestCancel();
        m_worker->wait(10000);
    }
}

void InstallProgressDialog::setupUi()
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->setContentsMargins(28, 24, 28, 20);
    vl->setSpacing(16);

    // 标题
    QLabel *title = new QLabel("正在部署: " + m_config.friendlyName);
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
    QLabel *logTitle = new QLabel("部署日志");
    logTitle->setObjectName("dialogLabel");
    vl->addWidget(logTitle);

    m_logEdit = new QTextEdit;
    m_logEdit->setObjectName("progressLogEdit");
    m_logEdit->setReadOnly(true);
    m_logEdit->setMinimumHeight(160);
    vl->addWidget(m_logEdit, 1);

    // 状态标签
    m_statusLabel = new QLabel("下载安装中...");
    m_statusLabel->setObjectName("progressStatusLabel");
    m_statusLabel->setProperty("status", "normal");
    vl->addWidget(m_statusLabel);

    // 取消/中止按钮
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    m_cancelBtn = new QPushButton("中止部署");
    m_cancelBtn->setFixedHeight(36);
    m_cancelBtn->setObjectName("progressCancelBtn");
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(m_cancelBtn, &QPushButton::clicked, this, &InstallProgressDialog::onCancel);
    btnRow->addWidget(m_cancelBtn);

    vl->addLayout(btnRow);
}

void InstallProgressDialog::buildStepIndicator(int total)
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
    labels << "下载安装";
    if (m_config.createAccount) {
        labels << "系统初始化" << "新建用户" << "设置密码" << "登录配置";
    }
    if (!m_config.isDefaultPath) {
        labels << "停止原实例" << "打包导出" << "注销清理" << "迁移导入" << "清理临时";
    }

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

void InstallProgressDialog::setStepActive(int step)
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

void InstallProgressDialog::onStepChanged(int step, int total, const QString &desc)
{
    if (m_stepDots.isEmpty()) {
        buildStepIndicator(total);
    }
    setStepActive(step);
    m_stepLabel->setText(QString("[步骤 %1/%2] %3").arg(step).arg(total).arg(desc));
}

void InstallProgressDialog::onLogMessage(const QString &msg)
{
    m_logEdit->insertPlainText(msg);
    m_logEdit->verticalScrollBar()->setValue(m_logEdit->verticalScrollBar()->maximum());
}

void InstallProgressDialog::onProgressValue(int percent)
{
    m_progressBar->setValue(percent);
}

void InstallProgressDialog::onFinished(bool success, const QString &message)
{
    m_finished = true;
    m_cancelBtn->setText("完成并关闭");

    if (success) {
        m_statusLabel->setText("部署成功");
        m_statusLabel->setProperty("status", "success");
        QMessageBox::information(this, "部署成功", message, QMessageBox::Ok);
        accept();
    } else {
        m_statusLabel->setText("部署失败");
        m_statusLabel->setProperty("status", "error");
        QMessageBox::critical(this, "部署错误", message, QMessageBox::Ok);
        reject();
    }
}

void InstallProgressDialog::onCancel()
{
    if (m_finished) {
        if (m_statusLabel->text().contains("成功")) {
            accept();
        } else {
            reject();
        }
        return;
    }

    QMessageBox::StandardButton sb = QMessageBox::question(
        this, "确认中止", "部署过程已开始，中止操作可能导致系统残留，确认中止吗？",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (sb == QMessageBox::Yes) {
        m_cancelBtn->setEnabled(false);
        m_cancelBtn->setText("正在中止...");
        m_worker->requestCancel();
    }
}

void InstallProgressDialog::closeEvent(QCloseEvent *event)
{
    if (!m_finished) {
        onCancel();
        event->ignore();
    } else {
        QDialog::closeEvent(event);
    }
}

void InstallProgressDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        event->ignore();
    } else {
        QDialog::keyPressEvent(event);
    }
}
