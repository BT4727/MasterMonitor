#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QButtonGroup>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QList>
#include <QStringList>
#include <array>

class LogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget *parent = nullptr);

private:
    enum LogType {
        OperationLog = 0,
        ExceptionLog,
        LogTypeCount
    };

    void initLeftArea();
    void initRightArea();
    void initTables();
    void initConnect();

    void setLogType(LogType type);
    void updatePagination();

    LogType m_currentLogType = OperationLog;

    int m_pageSize = 10;

    // 两类日志分别保存分页状态
    std::array<int, LogTypeCount> m_currentPages{1, 1};
    std::array<int, LogTypeCount> m_totalPages{1, 1};

    // 暂时为空，后续填入数据库查询结果
    // 每一条 QStringList 对应一行的四列内容
    QList<QStringList> m_operationLogs;
    QList<QStringList> m_exceptionLogs;

    QHBoxLayout *m_mainLayout = nullptr;

    // 左侧按钮区域
    QWidget *m_leftWidget = nullptr;
    QVBoxLayout *m_leftLayout = nullptr;

    QPushButton *m_operationLogButton = nullptr;
    QPushButton *m_exceptionLogButton = nullptr;
    QButtonGroup *m_logTypeGroup = nullptr;

    // 右侧内容区域
    QWidget *m_rightWidget = nullptr;
    QVBoxLayout *m_rightLayout = nullptr;

    QWidget *m_tableWidget = nullptr;
    QStackedLayout *m_tableStack = nullptr;
    std::array<QTableWidget *, LogTypeCount> m_tables{};

    // 分页区域
    QWidget *m_pagerWidget = nullptr;
    QHBoxLayout *m_pagerLayout = nullptr;

    QPushButton *m_previousButton = nullptr;
    QLabel *m_pageLabel = nullptr;
    QPushButton *m_nextButton = nullptr;
};

#endif // LOGWIDGET_H
