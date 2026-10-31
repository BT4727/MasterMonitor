QT += widgets core gui sql multimedia multimediawidgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    model/adminusermodel.cpp \
    model/configfilemodel.cpp \
    model/eventrecordmodel.cpp \
    model/operationlogmodel.cpp \
    model/snapshotmodel.cpp \
    model/userconfigmodel.cpp \
    model/videorecordmodel.cpp \
    service/adminuserservice.cpp \
    service/monitorservice.cpp \
    service/logservice.cpp \
    service/monitortask.cpp \
    service/playbackservice.cpp \
    service/persondetection.cpp \
    service/processtask.cpp \
    service/recordmanager.cpp \
    service/recordtask.cpp \
    service/snapshotservice.cpp \
    service/storagemonitorservice.cpp \
    service/systemconfigservice.cpp \
    util/dbconn.cpp \
    util/winmanager.cpp \
    view/identifyinglabel.cpp \
    view/loginwidget.cpp \
    view/logwidget.cpp \
    view/mainwidget.cpp \
    view/snapshotwidget.cpp \
    view/reviewwidget.cpp \
    view/settingswidget.cpp


HEADERS += \
    model/adminusermodel.h \
    model/configfilemodel.h \
    model/eventrecordmodel.h \
    model/operationlogmodel.h \
    model/snapshotmodel.h \
    model/systemconfigdata.h \
    model/userconfigmodel.h \
    model/videorecordmodel.h \
    service/adminuserservice.h \
    service/monitorservice.h \
    service/logservice.h \
    service/monitortask.h \
    service/playbackservice.h \
    service/persondetection.h \
    service/processtask.h \
    service/recordmanager.h \
    service/recordtask.h \
    service/snapshotservice.h \
    service/storagemonitorservice.h \
    service/systemconfigservice.h \
    util/dbconn.h \
    util/winmanager.h \
    view/identifyinglabel.h \
    view/loginwidget.h \
    view/logwidget.h \
    view/mainwidget.h \
    view/snapshotwidget.h \
    view/reviewwidget.h \
    view/settingswidget.h


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
DESTDIR=$$PWD/bin

# 配置opencv环境
OPENCV_ROOT=E:/learn/ClassThree/OpenCVData/opencv
# OpenCV头文件
INCLUDEPATH+=$${OPENCV_ROOT}/include
# OpenCV库文件
LIBS+=-L$${OPENCV_ROOT}/x64/mingw/lib

# 必选核心库(基础功能+视频读写,依赖 FFmpeg)
LIBS += -llibopencv_core4100        # 核心模块(内存管理、数据结构)
LIBS += -llibopencv_imgproc4100     # 图像处理(滤波、缩放等)
LIBS += -llibopencv_videoio4100     # 视频 I/0(调用 FFmpeg 读写视频)
LIBS += -llibopencv_highgui4100     # 简单 GUI 显示(如 imshow)
LIBS += -llibopencv_imgcodecs4100   # 图像编解码(读写图片)
LIBS += -llibopencv_dnn4100         # 用于opencv访问Onnx模型的接口模块
## 可选扩展库(根据项目需求添加)
LIBS += -llibopencv_features2d4100 #特征检测(如 SIFT/SURF)
LIBS += -llibopencv_calib3d4100     # 相机标定、3D 重建
LIBS += -llibopencv_objdetect4100   # 目标检测(如 Haar 级联)
LIBS += -llibopencv_video4100       # Kalman目标轨迹预测

RESOURCES += \
    resources.qrc

DISTFILES += bin \
    .gitignore

TARGET = SmartMonitor