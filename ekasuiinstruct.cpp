#include <QDebug>
#include <QThread>
#include <QSqlDriver>
#include <QtConcurrent/QtConcurrent>

#include "ekasuiinstruct.h"
#include "instructworker.h"
#include "xmlsettings.h"
#include "connectorsdo.h"

QWaitCondition EkasuiInstruct::asuWait;
QMutex EkasuiInstruct::mutex;
bool EkasuiInstruct::asuDone(false);

EkasuiInstruct::EkasuiInstruct(const dBase &db, QObject *parent)
    : QObject(parent)
    , m_nThreadCount(0)
    , m_bNeedRestart(false)
    , m_bWaitingAsu(false)
    , m_database(db)
{
}

EkasuiInstruct::~EkasuiInstruct()
{
    emit stopAll();
}

void EkasuiInstruct::SetSettings(const XmlSettings &xmlSett)
{
    XmlSettings xmlOld;
    {
        QReadLocker lock(&m_lockSettings);
        xmlOld = m_xmlSettings;
    }

    XmlSettings::Node groupsOld = xmlOld.GetNode("ekasui/groups");
    XmlSettings::Node groups = xmlSett.GetNode("ekasui/groups");

    if (groupsOld != groups)
    {
        if (m_nThreadCount > 0)
        {
            // Потоки уже работают
            {
                QWriteLocker lock(&m_lockSettings);
                m_xmlSettings = xmlSett;
            }
            if (!m_bNeedRestart.load())
            {
                // Перезапустим все потоки
                m_bNeedRestart = true;
                StopWorking();
                return;
            }
            else
            {
                // Уже запущен процесс перезапуска потоков
            }
        }
        else
        {
            // Запустим выполнение потоков
            {
                QWriteLocker lock(&m_lockSettings);
                m_xmlSettings = xmlSett;
            }
            QtConcurrent::run(StartProcessed, this);
            //StartProcessed();
        }
    }

    static int nWork(0);
    int nNewWork = "true" == xmlSett.GetValue("ekasui/enabled")
            ? 1 : -1;
    QString strError;
    if (    nWork != nNewWork &&
            m_database.Connect(strError))
    {
        if (1 == nNewWork)
        {
            m_database.writeLog(0 == nWork
                                ? "Запуск обработки ЕКАСУИ"
                                : "Пришла команда запуска обработки ЕКАСУИ", dBase::EKASUI);
        }
        else
        {
            m_database.writeLog(0 == nWork
                                ? "Обработка ЕКАСУИ не требуется"
                                : "Пришла команда остановки обработки ЕКАСУИ", dBase::EKASUI);
        }
        nWork = nNewWork;
    }

    QWriteLocker lock(&m_lockSettings);
    m_xmlSettings = xmlSett;
}

void EkasuiInstruct::StartProcessed()
{
    m_bNeedRestart = false;

    if (m_bWaitingAsu.load())
        return;

    QString strError;
    if (m_database.Connect(strError))
        m_database.writeLog("Запуск обработки ЕКАСУИ", dBase::EKASUI);
    else
        qDebug() << "Запуск обработки ЕКАСУИ";

    XmlSettings xmlSett;
    {
        QReadLocker lock(&m_lockSettings);
        xmlSett = m_xmlSettings;
    }

    // Подождем, если необходимо, пока не появятся данные в sync_personal
    if (m_database.Connect(strError))
    {
        int nPrsCount = m_database.GetInt("select count(*) from sync_personal");
        if (    0 == nPrsCount &&
                "true" == xmlSett.GetValue("assh2/enabled") &&
                !asuDone)
        {
            m_bWaitingAsu = true;
            mutex.lock();
            asuWait.wait(&mutex);
            mutex.unlock();
            m_bWaitingAsu = false;
        }
        m_database.Close();
    }

    // Обновим настройки, т.к. за время ожидания они могли измениться
    {
        QReadLocker lock(&m_lockSettings);
        xmlSett = m_xmlSettings;
    }

    XmlSettings::Node node = xmlSett.GetNode("ekasui/groups");
    foreach (const XmlSettings::Node &nd, node.m_arNode)
    {
        if (m_bNeedRestart)
            break;

        if ("true" == nd.GetValue("enabled"))
        {
            int tmZone = QString(nd.GetValue("time_zone")).toInt();
            QList<unsigned> listDor;
            XmlSettings::Node dors = nd.GetNode("dor");
            foreach (const XmlSettings::Node &dor, dors.m_arNode)
            {
                unsigned dorKod = dor.m_strValue.toUInt();
                if (dorKod > 0)
                    listDor.push_back(dorKod);
            }
            if (listDor.size() > 0)
            {
                InstructWorker *worker = new InstructWorker(*this, m_database, listDor, tmZone);
                QThread *thread = new QThread;
                worker->moveToThread(thread);

                // При запуске потока вызовем нужную функцию
                connect(thread, SIGNAL(started()), worker, SLOT(process()));

                // При завершении поточной функции завершим и поток
                connect(worker, SIGNAL(finished()), thread, SLOT(quit()));

                // При получении сигнала остановки остановим все
                connect(this, SIGNAL(stopAll()), worker, SLOT(stop()), Qt::DirectConnection);

                // По завершени поточной функции обработаем это и у нас
                connect(worker, SIGNAL(finished()), this, SLOT(onWorkerFinished()), Qt::DirectConnection);

                // По завершении поточной функции удалим объект
                connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));

                // По завершении потока удалим сам объект потока
                connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

                thread->start();
            }
        }
    }
}

void EkasuiInstruct::StartProcessed(EkasuiInstruct *pObject)
{
    pObject->StartProcessed();
}

void EkasuiInstruct::StopWorking()
{
    QString strError;
    if (m_database.Connect(strError))
        m_database.writeLog("Остановка обработки ЕКАСУИ", dBase::EKASUI);
    else
        qDebug() << "Остановка обработки ЕКАСУИ";

    emit stopAll();
}

bool EkasuiInstruct::IsEnabled() const
{
    QReadLocker lock(&m_lockSettings);
    return "true" == m_xmlSettings.GetValue("enabled");
}

/*void EkasuiInstruct::StartMzd()
{
    XmlSettings xmlSett;
    QString strError;
    if (m_dbSettings.Connect(strError) &&
        xmlSett.Load(m_dbSettings))
    {
        QString strEn = xmlSett.GetValue("ekasui/enabled"),
                strServes = xmlSett.GetValue("ekasui/server");
        QList <unsigned> listDor = {17};
        InstructWorker worker(m_dbSettings, listDor, 0);
        worker.process();
    }
}*/

void EkasuiInstruct::dbNotify(const QString &strNotify)
{
    qDebug() << "Пришло событие от БД: " << strNotify;
}

void EkasuiInstruct::onWorkerFinished()
{
    if (m_nThreadCount.load() > 0)
        --m_nThreadCount;

    if (    0 == m_nThreadCount.load() &&
            m_bNeedRestart.load())
        QtConcurrent::run(StartProcessed, this);
}
