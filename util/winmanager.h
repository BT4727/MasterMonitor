#ifndef WINMANAGER_H
#define WINMANAGER_H

#include "../view/loginwidget.h"
#include "../view/mainwidget.h"
#include "../view/settingswidget.h"
class WinManager
{
public:
    static WinManager* getInstance();
    // 禁用拷贝构造和赋值运算
    WinManager(const WinManager&)=delete ;
    WinManager& operator=(const WinManager&)=delete ;

    LoginWidget loginWidget;
    MainWidget mainWidget;
    SettingsWidget settingsWidget;

private:
    WinManager();
    static WinManager* instance;
};

#endif // WINMANAGER_H
