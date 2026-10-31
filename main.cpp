#include <QApplication>
#include "util/winmanager.h"

int main(int argc, char *argv[])
{
    // 初始化Qt应用、数据库连接及窗口管理器并进入事件循环。
    QApplication a(argc, argv);
    WinManager::getInstance()->mainWidget.show();
    return QApplication::exec();
}
