#include "diskchart.h"
#include "diskdata.h"
#include "utils.h"

#include <QPieSeries>
#include <QPieSlice>
#include <QPieLegendMarker>

#include <QToolTip>
#include <QCursor>
#include <QDebug>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsSceneResizeEvent>

static const QList<QColor> s_palette = {
    QColor(0x5470C6),
    QColor(0x91CC75),
    QColor(0xFAC858),
    QColor(0xEE6666),
    QColor(0x73C0DE),
    QColor(0x3BA272),
    QColor(0xFC8452),
    QColor(0x9A60B4),
    QColor(0xEA7CCC),
};

DiskChart::DiskChart(QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QChart(QChart::ChartTypeCartesian, parent, wFlags)
{
    setBackgroundVisible(false);
    setPlotAreaBackgroundVisible(false);
}

void DiskChart::setData(const DiskItem &root)
{
    removeAllSeries();

    if (root.children.isEmpty())
        return;

    const qint64 rootTotal = root.totalSize();
    if (rootTotal == 0)
        return;

    const QColor bgColor = palette().color(QPalette::Window);
    const QPen slicePen(bgColor, 1);

    // Determine the maximum depth so we can size rings evenly.
    // We'll discover it as we go, using a two-pass approach:
    // first count levels, then build rings.

    struct AngleItem {
        const DiskItem *item;
        qreal startAngle;
        qreal endAngle;
        int colorIndex;   // index into s_palette (inherited from top-level ancestor)
        int lightenSteps;  // how many times to lighten the base color
        QString path;      // full path from root
    };

    // --- Pass 1: count depth levels ---
    // Depth 0 = root's children shown as-is
    // Depth 1 = children of folders at depth 0
    // etc.
    int maxDepth = 0;
    {
        QVector<const DiskItem *> cur;
        for (const DiskItem &child : root.children)
            cur.append(&child);

        while (!cur.isEmpty()) {
            QVector<const DiskItem *> next;
            for (const DiskItem *item : cur) {
                if (item->isFolder()) {
                    for (const DiskItem &child : item->children)
                        next.append(&child);
                }
            }
            if (!next.isEmpty())
                ++maxDepth;
            cur = next;
        }
    }

    const int numRings = maxDepth + 1;
    const qreal maxPieSize = 0.9;
    const qreal centerSize = 0.15;  // reserved for the total-size center circle
    const qreal ringWidth = (maxPieSize - centerSize) / numRings;

    // Helper: connect hover and click signals on a series
    auto connectSignals = [this, rootTotal](QPieSeries *seg) {
        connect(seg, &QPieSeries::hovered, this,
                [rootTotal](QPieSlice *slice, bool state) {
            if (state) {
                // Darken the slice
                if (!slice->property("originalColor").isValid())
                    slice->setProperty("originalColor", slice->brush().color());
                QColor dark = slice->property("originalColor").value<QColor>().darker(130);
                slice->setBrush(dark);

                qint64 itemSize = qint64(slice->value());
                qreal pct = (rootTotal > 0)
                    ? (qreal(itemSize) / rootTotal) * 100.0
                    : 0.0;
                QString name = slice->property("itemName").toString();
                QString tip = QString("%1\n%2\n%3%")
                    .arg(name)
                    .arg(formatSize(itemSize))
                    .arg(pct, 0, 'f', 1);
                QToolTip::showText(QCursor::pos(), tip);
            } else {
                // Restore original color
                QColor orig = slice->property("originalColor").value<QColor>();
                if (orig.isValid())
                    slice->setBrush(orig);
                QToolTip::hideText();
            }
        });

        connect(seg, &QPieSeries::clicked, this, [](QPieSlice *slice) {
            QString path = slice->property("itemPath").toString();
            qDebug() << path;
        });
    };

    // --- Center circle: graphics items showing total size ---
    m_centerSize = centerSize;

    // Clean up old center items if re-setting data
    if (m_centerCircle) {
        delete m_centerCircle;
        m_centerCircle = nullptr;
    }
    if (m_centerText) {
        delete m_centerText;
        m_centerText = nullptr;
    }

    QString sizeLabel = formatSize(rootTotal);

    m_centerCircle = new QGraphicsEllipseItem(this);
    m_centerCircle->setBrush(QColor(0x303030));
    m_centerCircle->setPen(Qt::NoPen);
    m_centerCircle->setZValue(10);

    m_centerText = new QGraphicsSimpleTextItem(this);
    m_centerText->setText(sizeLabel);
    m_centerText->setFont(QFont("Arial", 10, QFont::Bold));
    m_centerText->setBrush(Qt::white);
    m_centerText->setZValue(11);

    // --- Ring 0 (innermost data ring): root's direct children ---
    // A single QPieSeries covering the full 360°.
    QVector<AngleItem> currentLevel;
    {
        qreal holeSize = centerSize;
        qreal pieSize = holeSize + ringWidth;

        auto *ring0 = new QPieSeries;
        ring0->setHoleSize(holeSize);
        ring0->setPieSize(pieSize);

        qreal angle = 0.0;
        for (int i = 0; i < root.children.size(); ++i) {
            const DiskItem &child = root.children[i];
            qint64 childSize = child.totalSize();
            qreal span = (qreal(childSize) / rootTotal) * 360.0;
            int ci = i % s_palette.size();

            QString childPath = root.name.endsWith('/')
                ? root.name + child.name
                : root.name + "/" + child.name;

            auto *slice = new QPieSlice(child.name, childSize);
            slice->setProperty("itemName", child.name);
            slice->setProperty("itemPath", childPath);
            slice->setBrush(s_palette[ci]);
            slice->setPen(slicePen);
            slice->setLabelVisible(false);
            ring0->append(slice);

            currentLevel.append({&child, angle, angle + span, ci, 0, childPath});
            angle += span;
        }

        connectSignals(ring0);
        addSeries(ring0);
    }

    // --- Rings 1+ : for each folder in the previous level, show its children ---
    for (int depth = 1; depth < numRings; ++depth) {
        qreal holeSize = centerSize + depth * ringWidth;
        qreal pieSize = holeSize + ringWidth;

        QVector<AngleItem> nextLevel;

        for (const AngleItem &parent : currentLevel) {
            if (!parent.item->isFolder())
                continue;  // leaf files don't produce children in the next ring

            auto *seg = new QPieSeries;
            seg->setHoleSize(holeSize);
            seg->setPieSize(pieSize);
            seg->setPieStartAngle(parent.startAngle);
            seg->setPieEndAngle(parent.endAngle);

            qint64 parentSize = parent.item->totalSize();
            qreal childAngle = parent.startAngle;

            for (const DiskItem &child : parent.item->children) {
                qint64 childSize = child.totalSize();
                qreal childSpan = (parentSize > 0)
                    ? (qreal(childSize) / parentSize)
                        * (parent.endAngle - parent.startAngle)
                    : 0;

                QString childPath;
                if (parent.path == "/") {
                    childPath = parent.path + child.name;
                } else {
                    childPath = parent.path + "/" + child.name;
                }


                auto *slice = new QPieSlice(child.name, childSize);
                slice->setProperty("itemName", child.name);
                slice->setProperty("itemPath", childPath);
                QColor color = s_palette[parent.colorIndex];
                for (int l = 0; l < parent.lightenSteps + 1; ++l)
                    color = color.lighter(115);
                slice->setBrush(color);
                slice->setPen(slicePen);
                slice->setLabelVisible(false);
                seg->append(slice);

                nextLevel.append({&child, childAngle, childAngle + childSpan,
                                  parent.colorIndex, parent.lightenSteps + 1, childPath});
                childAngle += childSpan;
            }

            connectSignals(seg);
            addSeries(seg);
        }

        currentLevel = nextLevel;
    }

    // setTitle(root.name);
    legend()->setVisible(false);
    // setAnimationOptions(QChart::SeriesAnimations);

    updateCenterLabel();
}

void DiskChart::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    QChart::resizeEvent(event);
    updateCenterLabel();
}

void DiskChart::updateCenterLabel()
{
    if (!m_centerCircle || !m_centerText)
        return;

    const QRectF area = plotArea();
    if (area.isEmpty())
        return;

    // The pie series use the shorter dimension to size themselves
    qreal side = qMin(area.width(), area.height());
    qreal radius = (side / 2.0) * m_centerSize - 1;
    QPointF center = area.center();

    m_centerCircle->setRect(center.x() - radius, center.y() - radius,
                            radius * 2.0, radius * 2.0);

    // Center the text inside the circle
    QRectF textRect = m_centerText->boundingRect();
    m_centerText->setPos(center.x() - textRect.width() / 2.0,
                         center.y() - textRect.height() / 2.0);
}
