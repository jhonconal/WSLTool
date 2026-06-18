#pragma once
#include <QDialog>
#include "../core/migrationworker.h"

class QLabel;
class QProgressBar;
class QTextEdit;
class QPushButton;
class QWidget;

class MigrationProgressDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MigrationProgressDialog(const MigrationConfig &config,
                                      QWidget *parent = nullptr);
    ~MigrationProgressDialog();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onStepChanged(int step, int total, const QString &desc);
    void onLogMessage(const QString &msg);
    void onProgressValue(int percent);
    void onFinished(bool success, const QString &message);
    void onCancel();

private:
    void setupUi();
    void buildStepIndicator(int total);
    void setStepActive(int step);

    MigrationConfig  m_config;
    MigrationWorker *m_worker;
    bool             m_finished;

    QLabel       *m_stepLabel;
    QLabel       *m_statusLabel;
    QProgressBar *m_progressBar;
    QTextEdit    *m_logEdit;
    QPushButton  *m_cancelBtn;
    QWidget      *m_stepArea;
    QList<QLabel*> m_stepDots;
    QList<QLabel*> m_stepLabels;
};

