#ifndef DRIVESELECTORWIDGET_H
#define DRIVESELECTORWIDGET_H

#include <QWidget>

class DriveListModel;
class DriveItemDelegate;
class QListView;
class QPushButton;

class DriveSelectorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DriveSelectorWidget(QWidget *parent = nullptr);

signals:
    void scanRequested(const QString &path);

private slots:
    void onScanClicked(const QModelIndex &index);
    void onAddFolder();

private:
    DriveListModel *m_model = nullptr;
    DriveItemDelegate *m_delegate = nullptr;
    QListView *m_listView = nullptr;
    QPushButton *m_addFolderBtn = nullptr;
};

#endif // DRIVESELECTORWIDGET_H
