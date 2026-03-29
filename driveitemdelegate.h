#ifndef DRIVEITEMDELEGATE_H
#define DRIVEITEMDELEGATE_H

#include <QStyledItemDelegate>

class DriveItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DriveItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

signals:
    void scanClicked(const QModelIndex &index);

private:
    QRect scanButtonRect(const QStyleOptionViewItem &option) const;
};

#endif // DRIVEITEMDELEGATE_H
