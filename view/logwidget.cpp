#include "logwidget.h"
#include <QHeaderView>
#include <QAbstractItemView>
#include <QTableWidgetItem>
#include <QtGlobal>

LogWidget::LogWidget(QWidget *parent)
    : QWidget{parent}
{
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(12);

    initLeftArea();
    initRightArea();
    initConnect();

    // 左侧按钮 : 右侧内容 = 2 : 8
    m_mainLayout->addWidget(m_leftWidget, 2);
    m_mainLayout->addWidget(m_rightWidget, 8);

    setLogType(OperationLog);
}

void LogWidget::initLeftArea()
{
    m_leftWidget = new QWidget(this);
    m_leftLayout = new QVBoxLayout(m_leftWidget);
    m_leftLayout->setContentsMargins(0, 0, 0, 0);
    m_leftLayout->setSpacing(0);

    m_operationLogButton = new QPushButton(
        QStringLiteral("操作日志"), m_leftWidget);

    m_exceptionLogButton = new QPushButton(
        QStringLiteral("异常日志"), m_leftWidget);

    m_operationLogButton->setCheckable(true);
    m_exceptionLogButton->setCheckable(true);

    m_logTypeGroup = new QButtonGroup(this);
    m_logTypeGroup->setExclusive(true);
    m_logTypeGroup->addButton(
        m_operationLogButton, OperationLog);
    m_logTypeGroup->addButton(
        m_exceptionLogButton, ExceptionLog);

    // 两个按钮作为一组，在左侧整体居中
    m_leftLayout->addStretch(1);

    m_leftLayout->addWidget(
        m_operationLogButton, 0, Qt::AlignHCenter);

    m_leftLayout->addSpacing(32);

    m_leftLayout->addWidget(
        m_exceptionLogButton, 0, Qt::AlignHCenter);

    m_leftLayout->addStretch(1);
}

void LogWidget::initRightArea()
{
    m_rightWidget = new QWidget(this);
    m_rightLayout = new QVBoxLayout(m_rightWidget);
    m_rightLayout->setContentsMargins(0, 0, 0, 0);
    m_rightLayout->setSpacing(8);

    // 两类日志表格共用堆栈
    m_tableWidget = new QWidget(m_rightWidget);
    m_tableStack = new QStackedLayout(m_tableWidget);
    m_tableStack->setContentsMargins(0, 0, 0, 0);

    initTables();

    // 共用分页栏
    m_pagerWidget = new QWidget(m_rightWidget);
    m_pagerLayout = new QHBoxLayout(m_pagerWidget);
    m_pagerLayout->setContentsMargins(0, 0, 0, 0);
    m_pagerLayout->setSpacing(12);

    m_previousButton = new QPushButton(
        QStringLiteral("上一页"), m_pagerWidget);

    m_pageLabel = new QLabel(m_pagerWidget);
    m_pageLabel->setObjectName(
        QStringLiteral("pageIndicator"));
    m_pageLabel->setAlignment(Qt::AlignCenter);

    m_nextButton = new QPushButton(
        QStringLiteral("下一页"), m_pagerWidget);

    m_pagerLayout->addStretch();
    m_pagerLayout->addWidget(m_previousButton);
    m_pagerLayout->addWidget(m_pageLabel);
    m_pagerLayout->addWidget(m_nextButton);
    m_pagerLayout->addStretch();

    // 表格 : 分页 = 9 : 1
    m_rightLayout->addWidget(m_tableWidget, 9);
    m_rightLayout->addWidget(m_pagerWidget, 1);
}

void LogWidget::initTables()
{
    const std::array<QStringList, LogTypeCount> headers = {
        QStringList{
            QStringLiteral("日志ID"),
            QStringLiteral("日志操作"),
            QStringLiteral("操作员"),
            QStringLiteral("操作时间")
        },
        QStringList{
            QStringLiteral("事件ID"),
            QStringLiteral("事件类型"),
            QStringLiteral("关联视频路径"),
            QStringLiteral("触发时间")
        }
    };

    for (int i = 0; i < LogTypeCount; ++i) {
        QTableWidget *table = new QTableWidget(
            0, 4, m_tableWidget);

        table->setHorizontalHeaderLabels(headers[i]);

        // 显示表头，隐藏左侧行号
        table->horizontalHeader()->setVisible(true);
        table->verticalHeader()->setVisible(false);

        // 暂时四列均分宽度
        table->horizontalHeader()->setSectionResizeMode(
            QHeaderView::Stretch);

        table->setSelectionBehavior(
            QAbstractItemView::SelectRows);
        table->setSelectionMode(
            QAbstractItemView::SingleSelection);
        table->setEditTriggers(
            QAbstractItemView::NoEditTriggers);

        table->setWordWrap(false);

        table->setShowGrid(true);
        table->setGridStyle(Qt::SolidLine);

        m_tables[i] = table;
        m_tableStack->addWidget(table);
    }
}

void LogWidget::initConnect()
{
    connect(m_operationLogButton,
            &QPushButton::clicked,
            this,
            [this]() {
                setLogType(OperationLog);
            });

    connect(m_exceptionLogButton,
            &QPushButton::clicked,
            this,
            [this]() {
                setLogType(ExceptionLog);
            });

    connect(m_previousButton,
            &QPushButton::clicked,
            this,
            [this]() {
                int &currentPage =
                    m_currentPages[m_currentLogType];

                if (currentPage > 1) {
                    --currentPage;
                    updatePagination();
                }
            });

    connect(m_nextButton,
            &QPushButton::clicked,
            this,
            [this]() {
                int &currentPage =
                    m_currentPages[m_currentLogType];

                if (currentPage
                    < m_totalPages[m_currentLogType]) {
                    ++currentPage;
                    updatePagination();
                }
            });
}

void LogWidget::setLogType(LogType type)
{
    m_currentLogType = type;
    m_tableStack->setCurrentIndex(type);

    m_logTypeGroup->button(type)->setChecked(true);

    updatePagination();
}

void LogWidget::updatePagination()
{
    m_pageSize = qMax(1, m_pageSize);

    // 选择当前日志类型对应的数据
    const QList<QStringList> &records =
        m_currentLogType == OperationLog
            ? m_operationLogs
            : m_exceptionLogs;

    const int totalCount =
        static_cast<int>(records.size());

    int &currentPage =
        m_currentPages[m_currentLogType];

    int &totalPages =
        m_totalPages[m_currentLogType];

    // 向上取整，空数据时保留 1/1
    totalPages = qMax(
        1,
        totalCount / m_pageSize
            + (totalCount % m_pageSize != 0));

    currentPage = qBound(1, currentPage, totalPages);

    m_pageLabel->setText(
        QStringLiteral("%1/%2")
            .arg(currentPage)
            .arg(totalPages));

    m_previousButton->setEnabled(currentPage > 1);
    m_nextButton->setEnabled(currentPage < totalPages);

    const int firstIndex =
        (currentPage - 1) * m_pageSize;

    const int rowCount = qMax(
        0, qMin(m_pageSize, totalCount - firstIndex));

    QTableWidget *table = m_tables[m_currentLogType];

    // 清除上一页内容，不清除表头
    table->setRowCount(0);
    table->setRowCount(rowCount);

    for (int row = 0; row < rowCount; ++row) {
        const QStringList &record =
            records.at(firstIndex + row);

        for (int column = 0; column < 4; ++column) {
            // 数据不足四列时，对应单元格留空
            QTableWidgetItem *item = new QTableWidgetItem(
                record.value(column));

            item->setToolTip(record.value(column));
            table->setItem(row, column, item);
        }
    }
}
