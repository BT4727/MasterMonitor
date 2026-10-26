#ifndef MOTIONREVIEWWIDGET_H
#define MOTIONREVIEWWIDGET_H

#include <QWidget>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPixmap>
#include <QEvent>

class MotionReviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MotionReviewWidget(QWidget *parent = nullptr);

signals:
    void screenshotRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void initLeftArea();
    void initVideoList();
    void initRightArea();
    void initConnect();

    void updateVideoPagination();
    void updateVideoColumnWidths();

    // 模拟总量与每页容量分别设置
    int m_totalVideoCount = 17;
    int m_videoPageSize = 4;
    int m_videoCurrentPage = 1;
    int m_videoTotalPages = 1;

    QPixmap m_videoThumbnail;
    QPixmap m_blackMonitorImage;

    QHBoxLayout *m_mainLayout = nullptr;

    // 左侧区域
    QWidget *m_leftWidget = nullptr;
    QVBoxLayout *m_leftLayout = nullptr;

    QWidget *m_filterWidget = nullptr;
    QHBoxLayout *m_filterLayout = nullptr;
    QDateTimeEdit *m_dateTimeEdit = nullptr;
    QComboBox *m_channelComboBox = nullptr;

    QTableWidget *m_videoTable = nullptr;

    QWidget *m_videoPagerWidget = nullptr;
    QHBoxLayout *m_videoPagerLayout = nullptr;
    QPushButton *m_videoPreviousButton = nullptr;
    QLabel *m_videoPageLabel = nullptr;
    QPushButton *m_videoNextButton = nullptr;

    // 右侧区域
    QWidget *m_rightWidget = nullptr;
    QVBoxLayout *m_rightLayout = nullptr;
    QLabel *m_videoDisplayLabel = nullptr;

    QWidget *m_videoControlsWidget = nullptr;
    QHBoxLayout *m_videoControlsLayout = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_screenshotButton = nullptr;
    QComboBox *m_speedComboBox = nullptr;
};

#endif // MOTIONREVIEWWIDGET_H
