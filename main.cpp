#include <QApplication>
#include "util/winmanager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WinManager::getInstance()->mainWidget.show();
    return QApplication::exec();
}
