#ifndef DBCONN_H
#define DBCONN_H
#include <QDebug>
#include <QtSql/QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QCryptographicHash>

class DBConn
{
public:
    static DBConn* getInstance();
    static void releaseInstance();
    DBConn(const DBConn&)=delete ;
    DBConn& operator=(const DBConn&)=delete ;
    QSqlDatabase getDb() const;
    QString getMd5(QString &str);
    static void configureDatabase(QSqlDatabase &database);
private:
    DBConn();
    ~DBConn();
    static DBConn* s_instance;
    QSqlDatabase db;
};

#endif // DBCONN_H
