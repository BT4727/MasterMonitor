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
    int totalVideoCount = 17;
    int videoPageSize = 4;
    int videoCurrentPage = 1;
    int videoTotalPages = 1;

    QPixmap videoThumbnail;
    QPixmap blackMonitorImage;

    QHBoxLayout *mainLayout = nullptr;

    // 左侧区域
    QWidget *leftWidget = nullptr;
    QVBoxLayout *leftLayout = nullptr;

    QWidget *filterWidget = nullptr;
    QHBoxLayout *filterLayout = nullptr;
    QDateTimeEdit *dateTimeEdit = nullptr;
    QComboBox *channelComboBox = nullptr;

    QTableWidget *videoTable = nullptr;

    QWidget *videoPagerWidget = nullptr;
    QHBoxLayout *videoPagerLayout = nullptr;
    QPushButton *videoPreviousButton = nullptr;
    QLabel *videoPageLabel = nullptr;
    QPushButton *videoNextButton = nullptr;

    // 右侧区域
    QWidget *rightWidget = nullptr;
    QVBoxLayout *rightLayout = nullptr;
    QLabel *videoDisplayLabel = nullptr;

    QWidget *videoControlsWidget = nullptr;
    QHBoxLayout *videoControlsLayout = nullptr;
    QPushButton *pauseButton = nullptr;
    QPushButton *screenshotButton = nullptr;
    QComboBox *speedComboBox = nullptr;
};

#endif // MOTIONREVIEWWIDGET_H
