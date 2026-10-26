#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QButtonGroup>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QStackedLayout>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <array>

#include "../service/logservice.h"

class LogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget *parent = nullptr);
    void setCurrentUser(const QString &username);

signals:
    void relatedVideoRequested(qulonglong videoId, qint64 offsetMs);

protected:
    void showEvent(QShowEvent *event) override;

private:
    enum LogType {
        OperationLog = 0,
        ExceptionLog,
        LogTypeCount
    };

    void initLeftArea();
    void initRightArea();
    void initFilterArea();
    void initTables();
    void initPagerArea();
    void initConnect();
    void setLogType(LogType type);
    void refreshCurrentPage();
    void updatePagination(int totalCount);
    void fillOperationTable();
    void fillExceptionTable();
    void queryLogs();
    void resetFilter();
    void deleteSelectedLogs();
    void deleteLogsBeforeThirtyDays();
    void deleteLogsInRange();
    void openRelatedVideo(int row, int column);
    QVector<qulonglong> selectedLogIds() const;
    QString operationTypeName(const QString &type) const;
    QString eventTypeName(const QString &type) const;
    bool confirmDelete(const QString &message) const;
    void recordDeleteResult(bool success, const QString &targetType,
                            int count, const QString &detail);

    static constexpr int PageSize = 10;

    QString currentUsername;
    bool useDefaultTimeRange = true;
    LogType currentLogType = OperationLog;
    std::array<int, LogTypeCount> currentPages{1, 1};
    std::array<int, LogTypeCount> totalPages{1, 1};
    QVector<OperationLogInfo> operationLogs;
    QVector<EventLogInfo> eventLogs;
    LogService logService;

    QHBoxLayout *mainLayout = nullptr;
    QWidget *leftWidget = nullptr;
    QVBoxLayout *leftLayout = nullptr;
    QPushButton *operationLogButton = nullptr;
    QPushButton *exceptionLogButton = nullptr;
    QButtonGroup *logTypeGroup = nullptr;

    QWidget *rightWidget = nullptr;
    QVBoxLayout *rightLayout = nullptr;
    QWidget *filterWidget = nullptr;
    QHBoxLayout *filterLayout = nullptr;
    QDateTimeEdit *startDateTimeEdit = nullptr;
    QDateTimeEdit *endDateTimeEdit = nullptr;
    QComboBox *channelComboBox = nullptr;
    QPushButton *queryButton = nullptr;
    QPushButton *resetButton = nullptr;

    QWidget *tableWidget = nullptr;
    QStackedLayout *tableStack = nullptr;
    std::array<QTableWidget *, LogTypeCount> tables{};

    QWidget *pagerWidget = nullptr;
    QHBoxLayout *pagerLayout = nullptr;
    QPushButton *previousButton = nullptr;
    QLabel *pageLabel = nullptr;
    QPushButton *nextButton = nullptr;
    QPushButton *relatedButton = nullptr;
    QPushButton *deleteSelectedButton = nullptr;
    QPushButton *deleteThirtyDaysButton = nullptr;
    QPushButton *deleteRangeButton = nullptr;
};

#endif // LOGWIDGET_H
