#include "dbconn.h"

DBConn* DBConn::s_instance=nullptr;
DBConn *DBConn::getInstance()
{
    if(DBConn::s_instance==nullptr){
        DBConn::s_instance=new DBConn;
    }
    return DBConn::s_instance;
}

void DBConn::releaseInstance()
{
    if(DBConn::s_instance!=nullptr){
        delete DBConn::s_instance;
        DBConn::s_instance=nullptr;
    }
}

DBConn::DBConn()
{
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
    // 释放数据库连接
    qDebug()<<"释放数据库连接";
    if(db.isOpen()){
        db.close(); // 断开数据库连接
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);// 移除默认连接
    }
}

void DBConn::configureDatabase(QSqlDatabase &database)
{
    database.setHostName("localhost");
    database.setPort(3306);
    database.setUserName("root");
    database.setPassword("123456");
    database.setDatabaseName("local_security_monitor");
}

QSqlDatabase DBConn::getDb() const
{
    return db;
}

QString DBConn::getMd5(QString &str){
    // 1. 将字符串转换位utf-8的字节数组（避免中文乱码）
    QByteArray byteArr = str.toUtf8();
    // 2. 计算MD5的哈希值
    QByteArray md5Arr = QCryptographicHash::hash(byteArr,QCryptographicHash::Md5);
    // 3. 转为十六进制字符串
    QString md5Str=md5Arr.toHex();
    return md5Str;
}
