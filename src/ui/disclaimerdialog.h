#pragma once
#include <QDialog>

class DisclaimerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DisclaimerDialog(QWidget *parent = nullptr);
    static bool shouldShow(); // 检查是否需要显示（首次运行）
    static void markAsShown(); // 标记已显示

private:
    void setupUi();
};

