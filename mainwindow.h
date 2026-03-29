#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class DiskChart;
class DriveSelectorWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void showChart(const QString &path);
    void showSelector();

private:
    void loadFile(const QString &path);

    Ui::MainWindow *ui;
    DiskChart *m_chart = nullptr;
    DriveSelectorWidget *m_selector = nullptr;
};
#endif // MAINWINDOW_H
