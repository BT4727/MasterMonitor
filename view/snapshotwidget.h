#ifndef SNAPSHOTWIDGET_H
#define SNAPSHOTWIDGET_H

#include "reviewwidget.h"

class SnapshotWidget : public ReviewWidget
{
    Q_OBJECT
public:
    explicit SnapshotWidget(QWidget *parent = nullptr);
};

#endif
