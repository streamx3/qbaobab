#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "diskchart.h"
#include "diskdata.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_chart = new DiskChart;
    ui->chartView->setChart(m_chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);

    loadFile("C:/Users/user/Documents/GitHub/qbaobab/data/scan01.json");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadFile(const QString &path)
{
    DiskItem root = DiskItem::fromJsonFile(path);
    m_chart->setData(root);
}
