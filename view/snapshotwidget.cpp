#include "snapshotwidget.h"

SnapshotWidget::SnapshotWidget(QWidget *parent)
    : ReviewWidget(parent, SnapshotReview)
{
    // 复用回放页面的图片预览与管理功能。
}
