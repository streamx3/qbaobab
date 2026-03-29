#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "diskchart.h"
#include "diskdata.h"
#include "driveselectorwidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Chart view (page 1 in stacked widget — already in .ui)
    m_chart = new DiskChart;
    ui->chartView->setChart(m_chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);

    // Drive selector (page 0 in stacked widget — inserted programmatically)
    m_selector = new DriveSelectorWidget;
    ui->stackedWidget->insertWidget(0, m_selector);
    ui->stackedWidget->setCurrentIndex(0);

    connect(m_selector, &DriveSelectorWidget::scanRequested,
            this, &MainWindow::showChart);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showChart(const QString &path)
{
    Q_UNUSED(path)
    // For now, load test data regardless of selected path
    loadFile("C:/Users/user/Documents/GitHub/qbaobab/data/scan01.json");
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::showSelector()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::loadFile(const QString &path)
{
    DiskItem root = DiskItem::fromJsonFile(path);
    m_chart->setData(root);
}
