#include "winmanager.h"
WinManager* WinManager::instance=nullptr;
WinManager *WinManager::getInstance()
{
    // 按需创建并返回窗口管理器单例。
    if(WinManager::instance==nullptr){
        WinManager::instance=new WinManager;
    }
    return WinManager::instance;
}
