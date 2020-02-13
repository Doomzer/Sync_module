#include <QSqlError>
#include <QDebug>
#include <QDomDocument>

#include "dbsettings.h"


std::atomic_int dBase::dbCount(0);

dBase::dBase()
    : dbPort(0)
{
}

dBase::dBase(const dBase &obj)
    : dbPort(0)
{
    set(/*obj.db,*/ obj.dbPort, obj.dbName, obj.host, obj.usr, obj.pwd, obj.driver);
}

void dBase::set(/*QSqlDatabase _db,*/ int _dbPort, QString _dbName, QString _host, QString _usr, QString _pwd, QString _driver)
{
    //db = _db;
    dbPort = _dbPort;
    dbName = _dbName;
    host = _host;
    usr = _usr;
    pwd = _pwd;
    driver = _driver;

}

bool dBase::Connect(QString &error)
{
    if (db.isOpen())
        return true;

    QString /*error,*/ conName;
    if(driver == "QODBC3")
        conName = "From";
    else
        conName = "To_" + QString::number(dbCount++);

    db = QSqlDatabase::addDatabase(driver,conName);
    if(driver == "QODBC3")
    {
        //db.db.setDatabaseName(QString("DRIVER={SQL Server};""SERVER=%1;DATABASE=%2;Persist Security Info=true;""uid=%3;pwd=%4").arg(db.host, db.dbName, db.usr, db.pwd));
        db.setDatabaseName(QString("DRIVER={Microsoft ODBC Driver 17 for SQL Server};""SERVER=%1;DATABASE=%2;Persist Security Info=true;""uid=%3;pwd=%4")
                           .arg(host, dbName, usr, pwd));
    }
    else
    {
        db.setHostName(host);
        db.setPort(dbPort);
        db.setDatabaseName(dbName);
        db.setUserName(usr);
        db.setPassword(pwd);
    }
    if (!db.open())
    {
        error = db.lastError().text();
        //writeLog(QString("Не удалось подключиться к базе %1: " + error).arg(dbName));
        //qDebug() << error;
        return false;
    }
    //qDebug() << conName;
    return true;
}

void dBase::Close()
{
    db.close();
}

void dBase::writeLog(const QString &msg, dBase::Error_type error_num)
{
    QString execString = "INSERT INTO log_module (time, module_id, code, message) VALUES (now(), %1, %2, '%3')";
    execString = execString.arg(1);
    execString = execString.arg(error_num);
    execString = execString.arg(msg);
    QSqlQuery qr(db);
    if(!qr.exec(execString))
    {
        //qDebug() << execString << " - FAILED";
    }
    qDebug() << msg;
}

std::vector<QString> dBase::getDocform()
{
    std::vector<QString> result{"","","",""};
    QString execString = "SELECT cast(convert_from(shablon, 'UTF8') as varchar) FROM docform WHERE docform_id = 10001";
    QSqlQuery qr(db);

    if(!qr.exec(execString))
    {
        writeLog(QString("Не удалось выполнить запрос %1").arg(execString), MOD);
    }
    else
    {
        while (qr.next())
        {
            QDomDocument doc;
            doc.setContent(qr.value(0).toString());
            QDomElement root = doc.documentElement();
            QDomNode node = root.firstChild();
            while (!node.isNull())
            {
              if (node.toElement().tagName() == "assh2")
              {
                  QDomNode nodeChild = node.firstChild();
                  while (!nodeChild.isNull())
                  {
                      if (nodeChild.toElement().tagName() == "server")
                      {
                          result[0] = nodeChild.toElement().text();
                      }
                      if (nodeChild.toElement().tagName() == "enabled")
                      {
                        result[1] = nodeChild.toElement().text();
                      }
                      nodeChild = nodeChild.nextSibling();
                  }
              }
              if (node.toElement().tagName() == "ekasui")
              {
                  QDomNode nodeChild = node.firstChild();
                  while (!nodeChild.isNull())
                  {
                      if (nodeChild.toElement().tagName() == "server")
                      {
                        result[2] = nodeChild.toElement().text();
                      }
                      if (nodeChild.toElement().tagName() == "enabled")
                      {
                        result[3] = nodeChild.toElement().text();
                      }
                      nodeChild = nodeChild.nextSibling();
                  }
              }
              node = node.nextSibling();
            }
        }
    }
    return result;
}

int dBase::GetInt(QString strSQL)
{
    QString strError;
    if (Connect(strError))
    {
        QSqlQuery qr(db);
        if (qr.exec(strSQL) &&
            qr.first())
            return qr.value(0).toInt();
    }

    return -1;
}
