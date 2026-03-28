#ifndef DISKCHART_H
#define DISKCHART_H

#include <QChart>

QT_FORWARD_DECLARE_CLASS(QGraphicsEllipseItem)
QT_FORWARD_DECLARE_CLASS(QGraphicsSimpleTextItem)

struct DiskItem;

class DiskChart : public QChart
{
public:
    explicit DiskChart(QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = {});

    void setData(const DiskItem &root);

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event) override;

private:
    void updateCenterLabel();

    QGraphicsEllipseItem *m_centerCircle = nullptr;
    QGraphicsSimpleTextItem *m_centerText = nullptr;
    qreal m_centerSize = 0.15;
};

#endif // DISKCHART_H
