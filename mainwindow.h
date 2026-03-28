#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class DiskChart;

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

private:
    void loadFile(const QString &path);

    Ui::MainWindow *ui;
    DiskChart *m_chart = nullptr;
};
#endif // MAINWINDOW_H
