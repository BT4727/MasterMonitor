#include "winmanager.h"
WinManager* WinManager::instance=nullptr;
WinManager *WinManager::getInstance()
{
    if(WinManager::instance==nullptr){
        WinManager::instance=new WinManager;
    }
    return WinManager::instance;
}

WinManager::WinManager()
{

}
