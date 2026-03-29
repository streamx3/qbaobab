#include "driveitemdelegate.h"
#include "drivelistmodel.h"
#include "utils.h"

#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QStyleOptionButton>
#include <QStyleOptionProgressBar>

static const int kRowHeight = 64;
static const int kIconSize = 48;
static const int kPadding = 8;
static const int kIconTextGap = 12;
static const int kBtnWidth = 70;
static const int kBtnHeight = 30;
static const int kProgressBarWidth = 150;
static const int kProgressBarHeight = 16;

DriveItemDelegate::DriveItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize DriveItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(0, kRowHeight);
}

QRect DriveItemDelegate::scanButtonRect(const QStyleOptionViewItem &option) const
{
    int btnX = option.rect.right() - kPadding - kBtnWidth;
    int btnY = option.rect.top() + (option.rect.height() - kBtnHeight) / 2;
    return QRect(btnX, btnY, kBtnWidth, kBtnHeight);
}

void DriveItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    painter->save();

    // Background
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);

    const QRect rect = option.rect;
    const int type = index.data(DriveListModel::TypeRole).toInt();
    const QString name = index.data(DriveListModel::DisplayNameRole).toString();
    const qint64 totalBytes = index.data(DriveListModel::TotalBytesRole).toLongLong();
    const qint64 usedBytes = index.data(DriveListModel::UsedBytesRole).toLongLong();
    const QIcon icon = index.data(DriveListModel::IconRole).value<QIcon>();

    // --- Icon ---
    int iconX = rect.left() + kPadding;
    int iconY = rect.top() + (rect.height() - kIconSize) / 2;
    QRect iconRect(iconX, iconY, kIconSize, kIconSize);
    icon.paint(painter, iconRect);

    // --- Scan button ---
    QRect btnRect = scanButtonRect(option);
    QStyleOptionButton btnOpt;
    btnOpt.rect = btnRect;
    btnOpt.text = tr("Scan");
    btnOpt.state = QStyle::State_Enabled;
    style->drawControl(QStyle::CE_PushButton, &btnOpt, painter, option.widget);

    // --- Text area boundaries ---
    int textLeft = iconRect.right() + kIconTextGap;
    int textRight = btnRect.left() - kPadding;
    int topRowY = rect.top() + kPadding;
    int bottomRowY = rect.top() + rect.height() / 2 + 2;

    // If it's a drive, split the right portion for progress bar / used label
    int nameRight = textRight;
    int progressLeft = textRight;
    if (type == DriveEntry::Drive) {
        progressLeft = textRight - kProgressBarWidth;
        nameRight = progressLeft - kPadding;
    }

    // --- Top-left: display name (bold) ---
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->setPen(option.palette.color(QPalette::Text));
    QRect nameRect(textLeft, topRowY, nameRight - textLeft, rect.height() / 2 - kPadding);
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, name);

    // --- Bottom-left: total capacity ---
    QFont smallFont = painter->font();
    smallFont.setBold(false);
    smallFont.setPointSize(smallFont.pointSize() - 1);
    painter->setFont(smallFont);
    painter->setPen(option.palette.color(QPalette::PlaceholderText));
    QRect capRect(textLeft, bottomRowY, nameRight - textLeft, rect.height() / 2 - kPadding);
    painter->drawText(capRect, Qt::AlignLeft | Qt::AlignVCenter, formatSize(totalBytes));

    // --- Drive-specific: progress bar + used label ---
    if (type == DriveEntry::Drive && totalBytes > 0) {
        // Progress bar (top-right area)
        int pct = int((double(usedBytes) / double(totalBytes)) * 100.0);

        QStyleOptionProgressBar pbOpt;
        pbOpt.rect = QRect(progressLeft, topRowY + 2, kProgressBarWidth, kProgressBarHeight);
        pbOpt.minimum = 0;
        pbOpt.maximum = 100;
        pbOpt.progress = pct;
        pbOpt.textVisible = false;
        pbOpt.state = QStyle::State_Enabled;
        style->drawControl(QStyle::CE_ProgressBar, &pbOpt, painter, option.widget);

        // Used label (bottom-right area)
        QString usedLabel = (usedBytes > 0) ? formatSize(usedBytes) : QStringLiteral("0");
        QRect usedRect(progressLeft, bottomRowY, kProgressBarWidth, rect.height() / 2 - kPadding);
        painter->drawText(usedRect, Qt::AlignRight | Qt::AlignVCenter, usedLabel);
    }

    // --- Separator line (skip last row) ---
    const QAbstractItemModel *model = index.model();
    if (model && index.row() < model->rowCount() - 1) {
        painter->setPen(QPen(option.palette.color(QPalette::Mid), 1));
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    }

    painter->restore();
}

bool DriveItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (scanButtonRect(option).contains(me->pos())) {
            emit scanClicked(index);
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
