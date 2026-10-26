#include "logwidget.h"

#include <QAbstractItemView>
#include <QDate>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QTime>
#include <QVariant>
#include <QtGlobal>

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent)
{
    mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(12);

    initLeftArea();
    initRightArea();
    initConnect();

    mainLayout->addWidget(leftWidget, 2);
    mainLayout->addWidget(rightWidget, 8);
    setLogType(OperationLog);
}

void LogWidget::initLeftArea()
{
    leftWidget = new QWidget(this);
    leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    operationLogButton = new QPushButton(QStringLiteral("操作日志"), leftWidget);
    exceptionLogButton = new QPushButton(QStringLiteral("异常日志"), leftWidget);
    operationLogButton->setCheckable(true);
    exceptionLogButton->setCheckable(true);

    logTypeGroup = new QButtonGroup(this);
    logTypeGroup->setExclusive(true);
    logTypeGroup->addButton(operationLogButton, OperationLog);
    logTypeGroup->addButton(exceptionLogButton, ExceptionLog);

    leftLayout->addStretch(1);
    leftLayout->addWidget(operationLogButton, 0, Qt::AlignHCenter);
    leftLayout->addSpacing(32);
    leftLayout->addWidget(exceptionLogButton, 0, Qt::AlignHCenter);
    leftLayout->addStretch(1);
}

void LogWidget::initRightArea()
{
    rightWidget = new QWidget(this);
    rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    initFilterArea();

    tableWidget = new QWidget(rightWidget);
    tableStack = new QStackedLayout(tableWidget);
    tableStack->setContentsMargins(0, 0, 0, 0);
    initTables();
    initPagerArea();

    rightLayout->addWidget(filterWidget);
    rightLayout->addWidget(tableWidget, 1);
    rightLayout->addWidget(pagerWidget);
}

void LogWidget::initFilterArea()
{
    filterWidget = new QWidget(rightWidget);
    filterLayout = new QHBoxLayout(filterWidget);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(8);

    startDateTimeEdit = new QDateTimeEdit(filterWidget);
    endDateTimeEdit = new QDateTimeEdit(filterWidget);
    startDateTimeEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    endDateTimeEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    startDateTimeEdit->setCalendarPopup(true);
    endDateTimeEdit->setCalendarPopup(true);

    channelComboBox = new QComboBox(filterWidget);
    channelComboBox->addItem(QStringLiteral("全部通道"), 0);
    for (int channelNo = 1; channelNo <= 4; ++channelNo)
        channelComboBox->addItem(QStringLiteral("通道%1").arg(channelNo), channelNo);

    queryButton = new QPushButton(QStringLiteral("查询"), filterWidget);
    resetButton = new QPushButton(QStringLiteral("重置"), filterWidget);

    filterLayout->addWidget(new QLabel(QStringLiteral("开始时间"), filterWidget));
    filterLayout->addWidget(startDateTimeEdit);
    filterLayout->addWidget(new QLabel(QStringLiteral("结束时间"), filterWidget));
    filterLayout->addWidget(endDateTimeEdit);
    filterLayout->addWidget(channelComboBox);
    filterLayout->addWidget(queryButton);
    filterLayout->addWidget(resetButton);

    resetFilter();
}

void LogWidget::initTables()
{
    const std::array<QStringList, LogTypeCount> headers = {
        QStringList{
            QStringLiteral("日志ID"), QStringLiteral("操作用户"),
            QStringLiteral("操作时间"), QStringLiteral("操作类型"),
            QStringLiteral("操作结果"), QStringLiteral("描述")
        },
        QStringList{
            QStringLiteral("事件编号"), QStringLiteral("触发时间"),
            QStringLiteral("通道"), QStringLiteral("事件类型"),
            QStringLiteral("图片数量"), QStringLiteral("视频路径")
        }
    };

    for (int index = 0; index < LogTypeCount; ++index) {
        QTableWidget *table = new QTableWidget(0, 6, tableWidget);
        table->setHorizontalHeaderLabels(headers[index]);
        table->horizontalHeader()->setVisible(true);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setWordWrap(false);
        table->setShowGrid(true);
        table->setGridStyle(Qt::SolidLine);
        tables[index] = table;
        tableStack->addWidget(table);
    }
}

void LogWidget::initPagerArea()
{
    pagerWidget = new QWidget(rightWidget);
    pagerLayout = new QHBoxLayout(pagerWidget);
    pagerLayout->setContentsMargins(0, 0, 0, 0);
    pagerLayout->setSpacing(8);

    relatedButton = new QPushButton(QStringLiteral("查看关联"), pagerWidget);
    deleteSelectedButton = new QPushButton(QStringLiteral("删除所选"), pagerWidget);
    deleteThirtyDaysButton = new QPushButton(QStringLiteral("删除30天前"), pagerWidget);
    deleteRangeButton = new QPushButton(QStringLiteral("删除筛选范围"), pagerWidget);
    previousButton = new QPushButton(QStringLiteral("上一页"), pagerWidget);
    pageLabel = new QLabel(pagerWidget);
    pageLabel->setObjectName(QStringLiteral("pageIndicator"));
    pageLabel->setAlignment(Qt::AlignCenter);
    nextButton = new QPushButton(QStringLiteral("下一页"), pagerWidget);

    pagerLayout->addWidget(relatedButton);
    pagerLayout->addWidget(deleteSelectedButton);
    pagerLayout->addWidget(deleteThirtyDaysButton);
    pagerLayout->addWidget(deleteRangeButton);
    pagerLayout->addStretch();
    pagerLayout->addWidget(previousButton);
    pagerLayout->addWidget(pageLabel);
    pagerLayout->addWidget(nextButton);
}

void LogWidget::initConnect()
{
    connect(operationLogButton, &QPushButton::clicked,
            this, [this]() { setLogType(OperationLog); });
    connect(exceptionLogButton, &QPushButton::clicked,
            this, [this]() { setLogType(ExceptionLog); });
    connect(queryButton, &QPushButton::clicked, this, &LogWidget::queryLogs);
    connect(resetButton, &QPushButton::clicked, this, &LogWidget::resetFilter);
    connect(deleteSelectedButton, &QPushButton::clicked,
            this, &LogWidget::deleteSelectedLogs);
    connect(deleteThirtyDaysButton, &QPushButton::clicked,
            this, &LogWidget::deleteLogsBeforeThirtyDays);
    connect(deleteRangeButton, &QPushButton::clicked,
            this, &LogWidget::deleteLogsInRange);

    connect(previousButton, &QPushButton::clicked, this, [this]() {
        int &page = currentPages[currentLogType];
        if (page > 1) {
            --page;
            refreshCurrentPage();
        }
    });
    connect(nextButton, &QPushButton::clicked, this, [this]() {
        int &page = currentPages[currentLogType];
        if (page < totalPages[currentLogType]) {
            ++page;
            refreshCurrentPage();
        }
    });

    connect(tables[ExceptionLog], &QTableWidget::cellDoubleClicked,
            this, &LogWidget::openRelatedVideo);
    connect(relatedButton, &QPushButton::clicked, this, [this]() {
        openRelatedVideo(tables[ExceptionLog]->currentRow(), 0);
    });
}

void LogWidget::setCurrentUser(const QString &username)
{
    currentUsername = username.trimmed();
    currentPages = {1, 1};
    operationLogs.clear();
    eventLogs.clear();
    tables[OperationLog]->setRowCount(0);
    tables[ExceptionLog]->setRowCount(0);
}

void LogWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (useDefaultTimeRange)
        endDateTimeEdit->setDateTime(QDateTime::currentDateTime().addSecs(1));
    if (!currentUsername.isEmpty())
        refreshCurrentPage();
}

void LogWidget::setLogType(LogType type)
{
    currentLogType = type;
    tableStack->setCurrentIndex(type);
    logTypeGroup->button(type)->setChecked(true);
    relatedButton->setEnabled(type == ExceptionLog);
    if (!currentUsername.isEmpty())
        refreshCurrentPage();
    else
        updatePagination(0);
}

void LogWidget::queryLogs()
{
    useDefaultTimeRange = false;
    currentPages[currentLogType] = 1;
    refreshCurrentPage();
}

void LogWidget::resetFilter()
{
    useDefaultTimeRange = true;
    startDateTimeEdit->setDateTime(QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0)));
    endDateTimeEdit->setDateTime(QDateTime::currentDateTime().addSecs(1));
    channelComboBox->setCurrentIndex(0);
    currentPages[currentLogType] = 1;

    if (!currentUsername.isEmpty())
        refreshCurrentPage();
}

void LogWidget::refreshCurrentPage()
{
    if (currentUsername.isEmpty()) {
        updatePagination(0);
        return;
    }

    const QDateTime startTime = startDateTimeEdit->dateTime();
    const QDateTime endTime = endDateTimeEdit->dateTime();
    if (endTime <= startTime) {
        QMessageBox::warning(this, QStringLiteral("查询失败"),
                             QStringLiteral("结束时间必须晚于开始时间"));
        return;
    }

    const int channelNo = channelComboBox->currentData().toInt();
    int totalCount = 0;
    QString errorMessage;
    bool searched = false;

    if (currentLogType == OperationLog) {
        searched = logService.searchOperationLogs(
            startTime, endTime, channelNo, currentPages[currentLogType],
            PageSize, operationLogs, totalCount, errorMessage);
    } else {
        searched = logService.searchEventLogs(
            startTime, endTime, channelNo, currentPages[currentLogType],
            PageSize, eventLogs, totalCount, errorMessage);
    }

    if (!searched) {
        QMessageBox::warning(this, QStringLiteral("日志查询失败"), errorMessage);
        return;
    }

    updatePagination(totalCount);
    if (currentLogType == OperationLog)
        fillOperationTable();
    else
        fillExceptionTable();
}

void LogWidget::updatePagination(int totalCount)
{
    int &page = currentPages[currentLogType];
    int &pageCount = totalPages[currentLogType];
    pageCount = qMax(1, (totalCount + PageSize - 1) / PageSize);
    page = qBound(1, page, pageCount);
    pageLabel->setText(QStringLiteral("%1/%2").arg(page).arg(pageCount));
    previousButton->setEnabled(page > 1);
    nextButton->setEnabled(page < pageCount);
}

void LogWidget::fillOperationTable()
{
    QTableWidget *table = tables[OperationLog];
    table->setRowCount(operationLogs.size());

    for (int row = 0; row < operationLogs.size(); ++row) {
        const OperationLogInfo &log = operationLogs.at(row);
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(log.id));
        idItem->setData(Qt::UserRole, QVariant::fromValue(log.id));
        table->setItem(row, 0, idItem);
        table->setItem(row, 1, new QTableWidgetItem(log.username));
        table->setItem(row, 2, new QTableWidgetItem(
            log.createTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
        table->setItem(row, 3, new QTableWidgetItem(operationTypeName(log.operationType)));
        table->setItem(row, 4, new QTableWidgetItem(
            log.success ? QStringLiteral("成功") : QStringLiteral("失败")));
        QTableWidgetItem *descriptionItem = new QTableWidgetItem(log.description);
        descriptionItem->setToolTip(log.description);
        table->setItem(row, 5, descriptionItem);
    }
}

void LogWidget::fillExceptionTable()
{
    QTableWidget *table = tables[ExceptionLog];
    table->setRowCount(eventLogs.size());

    for (int row = 0; row < eventLogs.size(); ++row) {
        const EventLogInfo &event = eventLogs.at(row);
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(event.id));
        idItem->setData(Qt::UserRole, QVariant::fromValue(event.id));
        idItem->setData(Qt::UserRole + 1, QVariant::fromValue(event.videoId));
        table->setItem(row, 0, idItem);
        table->setItem(row, 1, new QTableWidgetItem(
            event.createTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))));
        table->setItem(row, 2, new QTableWidgetItem(
            event.channelNo == 0 ? QStringLiteral("系统")
                                 : QStringLiteral("通道%1").arg(event.channelNo)));
        table->setItem(row, 3, new QTableWidgetItem(eventTypeName(event.eventType)));
        table->setItem(row, 4, new QTableWidgetItem(QString::number(event.imageCount)));
        QTableWidgetItem *pathItem = new QTableWidgetItem(event.videoPath);
        pathItem->setToolTip(event.videoPath);
        table->setItem(row, 5, pathItem);
    }
}

QVector<qulonglong> LogWidget::selectedLogIds() const
{
    QVector<qulonglong> ids;
    const QModelIndexList rows = tables[currentLogType]->selectionModel()->selectedRows();
    for (const QModelIndex &rowIndex : rows) {
        QTableWidgetItem *item = tables[currentLogType]->item(rowIndex.row(), 0);
        if (item != nullptr)
            ids.append(item->data(Qt::UserRole).toULongLong());
    }
    return ids;
}

void LogWidget::deleteSelectedLogs()
{
    const QVector<qulonglong> ids = selectedLogIds();
    if (ids.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("删除日志"),
                                 QStringLiteral("请先选择需要删除的日志"));
        return;
    }

    if (!confirmDelete(QStringLiteral("确定删除选中的%1条日志吗？").arg(ids.size())))
        return;

    QString errorMessage;
    const bool deleted = currentLogType == OperationLog
        ? logService.deleteOperationLogs(ids, errorMessage)
        : logService.deleteEventLogs(ids, errorMessage);
    const QString targetType = currentLogType == OperationLog
        ? QStringLiteral("operation_log") : QStringLiteral("event");
    recordDeleteResult(deleted, targetType, ids.size(), errorMessage);

    if (!deleted) {
        QMessageBox::warning(this, QStringLiteral("删除失败"), errorMessage);
        return;
    }

    refreshCurrentPage();
}

void LogWidget::deleteLogsBeforeThirtyDays()
{
    if (!confirmDelete(QStringLiteral("确定删除30天前的全部当前类型日志吗？")))
        return;

    const QDateTime deleteTime = QDateTime::currentDateTime().addDays(-30);
    QString errorMessage;
    const bool deleted = currentLogType == OperationLog
        ? logService.deleteOperationLogsBefore(deleteTime, errorMessage)
        : logService.deleteEventLogsBefore(deleteTime, errorMessage);
    const QString targetType = currentLogType == OperationLog
        ? QStringLiteral("operation_log") : QStringLiteral("event");
    recordDeleteResult(deleted, targetType, 0,
                       deleted ? QStringLiteral("删除30天前日志") : errorMessage);

    if (!deleted) {
        QMessageBox::warning(this, QStringLiteral("删除失败"), errorMessage);
        return;
    }

    refreshCurrentPage();
}

void LogWidget::deleteLogsInRange()
{
    const QDateTime startTime = startDateTimeEdit->dateTime();
    const QDateTime endTime = endDateTimeEdit->dateTime();
    if (endTime <= startTime) {
        QMessageBox::warning(this, QStringLiteral("删除失败"),
                             QStringLiteral("结束时间必须晚于开始时间"));
        return;
    }

    if (!confirmDelete(QStringLiteral("确定删除当前时间和通道筛选范围内的日志吗？")))
        return;

    const int channelNo = channelComboBox->currentData().toInt();
    QString errorMessage;
    const bool deleted = currentLogType == OperationLog
        ? logService.deleteOperationLogsRange(startTime, endTime, channelNo, errorMessage)
        : logService.deleteEventLogsRange(startTime, endTime, channelNo, errorMessage);
    const QString targetType = currentLogType == OperationLog
        ? QStringLiteral("operation_log") : QStringLiteral("event");
    recordDeleteResult(deleted, targetType, 0,
                       deleted ? QStringLiteral("删除指定范围日志") : errorMessage);

    if (!deleted) {
        QMessageBox::warning(this, QStringLiteral("删除失败"), errorMessage);
        return;
    }

    refreshCurrentPage();
}

void LogWidget::openRelatedVideo(int row, int)
{
    if (currentLogType != ExceptionLog || row < 0)
        return;

    QTableWidgetItem *item = tables[ExceptionLog]->item(row, 0);
    if (item == nullptr)
        return;

    const qulonglong videoId = item->data(Qt::UserRole + 1).toULongLong();
    if (videoId == 0) {
        QMessageBox::information(this, QStringLiteral("关联查看"),
                                 QStringLiteral("该异常事件没有关联视频"));
        return;
    }

    emit relatedVideoRequested(videoId, 0);
}

QString LogWidget::operationTypeName(const QString &type) const
{
    if (type == QStringLiteral("login"))
        return QStringLiteral("登录");
    if (type == QStringLiteral("logout"))
        return QStringLiteral("退出");
    if (type == QStringLiteral("config"))
        return QStringLiteral("配置");
    if (type == QStringLiteral("playback"))
        return QStringLiteral("回放");
    if (type == QStringLiteral("snapshot"))
        return QStringLiteral("截图");
    if (type == QStringLiteral("delete"))
        return QStringLiteral("删除");
    if (type == QStringLiteral("export"))
        return QStringLiteral("导出");
    return type;
}

QString LogWidget::eventTypeName(const QString &type) const
{
    if (type == QStringLiteral("person_detected"))
        return QStringLiteral("检测到移动目标");
    if (type == QStringLiteral("disk_low"))
        return QStringLiteral("磁盘空间不足");
    return type;
}

bool LogWidget::confirmDelete(const QString &message) const
{
    return QMessageBox::question(
        const_cast<LogWidget *>(this), QStringLiteral("确认删除"), message,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes;
}

void LogWidget::recordDeleteResult(bool success, const QString &targetType,
                                   int count, const QString &detail)
{
    const int channelNo = channelComboBox->currentData().toInt();
    QString description = success
        ? QStringLiteral("删除%1条%2记录").arg(count).arg(targetType)
        : QStringLiteral("删除%1记录失败：%2").arg(targetType, detail);
    if (count == 0 && success)
        description = detail;

    logService.recordOperation(
        currentUsername, channelNo, QStringLiteral("delete"),
        targetType, 0, success, description);
}
