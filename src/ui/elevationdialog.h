#pragma once
#include <QDialog>

class ElevationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ElevationDialog(QWidget *parent = nullptr);
    bool shouldElevate() const { return m_shouldElevate; }

private:
    void setupUi();
    bool m_shouldElevate = false;
};
