#include "driveselectorwidget.h"
#include "drivelistmodel.h"
#include "driveitemdelegate.h"

#include <QFileDialog>
#include <QListView>
#include <QPushButton>
#include <QVBoxLayout>

DriveSelectorWidget::DriveSelectorWidget(QWidget *parent)
    : QWidget(parent)
{
    m_model = new DriveListModel(this);
    m_delegate = new DriveItemDelegate(this);

    m_listView = new QListView;
    m_listView->setModel(m_model);
    m_listView->setItemDelegate(m_delegate);
    m_listView->setSelectionMode(QAbstractItemView::NoSelection);
    m_listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listView->setFrameShape(QFrame::NoFrame);

    m_addFolderBtn = new QPushButton(tr("Add Folder"));

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_listView, 1);
    layout->addWidget(m_addFolderBtn, 0, Qt::AlignLeft);

    connect(m_delegate, &DriveItemDelegate::scanClicked,
            this, &DriveSelectorWidget::onScanClicked);
    connect(m_addFolderBtn, &QPushButton::clicked,
            this, &DriveSelectorWidget::onAddFolder);
}

void DriveSelectorWidget::onScanClicked(const QModelIndex &index)
{
    QString path = index.data(DriveListModel::PathRole).toString();
    emit scanRequested(path);
}

void DriveSelectorWidget::onAddFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Folder"));
    if (!dir.isEmpty())
        m_model->addUserFolder(dir);
}
