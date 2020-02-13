#ifndef DBSETTINGS_H
#define DBSETTINGS_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>

class dBase
{
public:
    static std::atomic_int dbCount;

    enum Error_type{MOD = 1, ASU, EKASUI, SDO};

    QSqlDatabase db;
    int dbPort;
    QString dbName;
    QString host;
    QString usr;
    QString pwd;
    QString driver;

    dBase();
    dBase(const dBase &obj);

    void set(/*QSqlDatabase _db,*/ int _dbPort, QString _dbName, QString _host, QString _usr, QString _pwd, QString _driver);

    bool Connect(QString &error);
    void Close();

    inline operator QSqlDatabase() { return db; }

    void writeLog(const QString &msg, Error_type error_num);

    std::vector<QString> getDocform();

    int GetInt(QString strSQL);
};

#endif // DBSETTINGS_H
