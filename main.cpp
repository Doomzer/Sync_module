#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QXmlStreamReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCryptographicHash>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QDomDocument>
#include <QtConcurrent/QtConcurrent>

#include "ekasuiinstruct.h"
#include "asuinstruct.h"
#include "instructworker.h"
#include "connectorsdo.h"
#include "dbsettings.h"
#include "sdoworker.h"
#include "xmlsettings.h"

#include "Ekasui/soapJpiServiceSOAPProxy.h"

#include "net.h"

#define BYTE unsigned char
#define SHORT short
#define UINT unsigned int



void startDaemon()
{
    QString path = "setFile.ini";

    #ifdef Q_OS_LINUX
        path = "/usr/share/nginx/html/config.ini";
    #endif

    QSettings sett(path, QSettings::IniFormat);
    dBase::dbCount = 0;
    QString ip = sett.value("db_server").toString();
    int port = sett.value("db_port").toInt();
    QString dbName = sett.value("db_name").toString();
    QString dbLog = sett.value("db_login").toString();
    QString dbPass = sett.value("db_password").toString();
    //ConnectorSdo::sdo = sett.value("sdo").toString();
    //QSqlDatabase dbTo;
    dBase dBaseTo;
    dBaseTo.set(/*dbTo,*/ port, dbName, ip, dbLog, dbPass, "QPSQL");

    //auto temp = GetSid();
    //getTable(temp, "21530903");

    //auto run1 = QtConcurrent::run(doAsu, dBaseTo);
    //auto run2 = QtConcurrent::run(doEkasui, dBaseTo);

    //AsuInstruct instr(dBaseTo);
    //instr.StartProcessed();

    //EkasuiInstruct ekasui(dBaseTo);
    //ekasui.StartProcessed();

    XmlSettings xmlSet;
    QString strError;
    SdoWorker *pSdoWorker = nullptr;
    AsuInstruct *pAsuWorker = nullptr;
    EkasuiInstruct ekasui(dBaseTo);
    while (true)
    {
        XmlSettings xml;
        if (    dBaseTo.Connect(strError) &&
                xml.Load(dBaseTo) &&
                xmlSet != xml)
        {
            // Изменились настройки, проверим, что не так

            // Проверим АСУ-Ш-2
            {
                XmlSettings::Node asuOld = xmlSet.GetNode("assh2");
                XmlSettings::Node asu = xml.GetNode("assh2");
                if (asuOld != asu)
                {
                    if (asuOld.GetValue("enabled") != asu.GetValue("enabled"))
                    {
                        if ("true" == asu.GetValue("enabled"))
                        {
                            dBaseTo.writeLog("Включена синхронизация с АСУ-Ш-2", dBase::ASU);

                            if (pAsuWorker)
                                pAsuWorker->stop();
                            pAsuWorker = new AsuInstruct(dBaseTo);
                        }
                        else
                        {
                            dBaseTo.writeLog("Синхронизация с АСУ-Ш-2 выключена", dBase::ASU);
                            if (pAsuWorker)
                            {
                                pAsuWorker->stop();
                                pAsuWorker = nullptr;
                            }
                        }
                    }
                }
            }

            // Проверим ЕКАСУ-И
            {
                XmlSettings::Node ekaOld = xmlSet.GetNode("ekasui");
                XmlSettings::Node eka = xml.GetNode("ekasui");
                if (ekaOld != eka)
                    ekasui.SetSettings(xml);
            }

            // Проверим СДО
            {
                XmlSettings::Node sdoOld = xmlSet.GetNode("sdo");
                XmlSettings::Node sdo = xml.GetNode("sdo");

                if (sdoOld != sdo)
                {
                    if (sdoOld.GetValue("enabled") != sdo.GetValue("enabled"))
                    {
                        if ("true" == sdo.GetValue("enabled"))
                        {
                            dBaseTo.writeLog("Включена синхронизация инструктажей в СДО", dBase::SDO);
                            if (pSdoWorker)
                                pSdoWorker->stop();
                            pSdoWorker = new SdoWorker(ekasui, dBaseTo);
                        }
                        else
                        {
                            dBaseTo.writeLog("Cинхронизация инструктажей в СДО выключена", dBase::SDO);
                            if (pSdoWorker)
                            {
                                pSdoWorker->stop();
                                pSdoWorker = nullptr;
                            }
                        }
                    }

                    if (pSdoWorker)
                        pSdoWorker->SetSettings(xml);
                }
            }

            xmlSet = xml;
        }

        QThread::sleep(10);
    }
}

int main(int argc, char *argv[])
{    
    QCoreApplication a(argc, argv);
    system("chcp 1251");
    startDaemon();
    return a.exec();
}
