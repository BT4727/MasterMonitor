#include "dbconn.h"

DBConn* DBConn::s_instance=nullptr;
DBConn *DBConn::getInstance()
{
    // 按需创建并返回数据库连接管理器单例。
    if(DBConn::s_instance==nullptr){
        DBConn::s_instance=new DBConn;
    }
    return DBConn::s_instance;
}

void DBConn::releaseInstance()
{
    // 释放数据库连接管理器单例。
    if(DBConn::s_instance!=nullptr){
        delete DBConn::s_instance;
        DBConn::s_instance=nullptr;
    }
}

DBConn::DBConn()
{
    // 注册MySQL驱动、配置连接参数并打开默认数据库连接。
    // 1. 注册mysql驱动
    db = QSqlDatabase::addDatabase("QMYSQL");
    // 2. 设置数据库连接参数
    configureDatabase(db);
    // 3. 打开数据库
    if(!db.open()){
        // 打开失败
        qDebug()<<"连接数据库失败"<<db.lastError().text();
        return;
    }
    qDebug()<<"数据库连接成功";
}

DBConn::~DBConn()
{
    // 关闭默认连接并从Qt连接池中移除。
    // 释放数据库连接
    qDebug()<<"释放数据库连接";
    if(db.isOpen()){
        db.close(); // 断开数据库连接
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);// 移除默认连接
    }
}

void DBConn::configureDatabase(QSqlDatabase &database)
{
    // 统一设置数据库主机、账户和库名参数。
    database.setHostName("localhost");
    database.setPort(3306);
    database.setUserName("root");
    database.setPassword("123456");
    database.setDatabaseName("local_security_monitor");
}

QSqlDatabase DBConn::getDb() const
{
    // 返回当前数据库连接句柄。
    return db;
}

QString DBConn::getMd5(QString &str){
    // 将UTF-8文本计算为MD5十六进制摘要。
    // 1. 将字符串转换位utf-8的字节数组（避免中文乱码）
    QByteArray byteArr = str.toUtf8();
    // 2. 计算MD5的哈希值
    QByteArray md5Arr = QCryptographicHash::hash(byteArr,QCryptographicHash::Md5);
    // 3. 转为十六进制字符串
    QString md5Str=md5Arr.toHex();
    return md5Str;
}
