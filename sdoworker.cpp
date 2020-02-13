#include "sdoworker.h"
#include "xmlsettings.h"
#include "connectorsdo.h"
#include "ekasuiinstruct.h"
#include "instructworker.h"
#include "xmlsettings.h"

#include <QThread>
#include <QDebug>
#include <QDateTime>
#include <QDate>
#include <QTime>

SdoWorker::SdoWorker(const EkasuiInstruct &instruct, const dBase &database, QObject *parent)
    : QObject(parent)
    , m_database(database)
    , m_bStopping(false)
    , m_instruct(instruct)
{
    // Запустим свой поток
    QThread *thread = new QThread;
    this->moveToThread(thread);

    // При запуске потока вызовем нужную функцию
    connect(thread, SIGNAL(started()), this, SLOT(process()));

    // При завершении поточной функции завершим и поток
    connect(this, SIGNAL(finished()), thread, SLOT(quit()));

    // По завершении поточной функции удалим объект
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));

    // По завершении потока удалим сам объект потока
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    thread->start();
}

SdoWorker::~SdoWorker()
{
    qDebug() << "SdoWorker::~SdoWorker()";
}

void SdoWorker::SetSettings(const XmlSettings &xml)
{
    QString strSdo = xml.GetValue("sdo/server");
    int nGroup = xml.GetValue("sdo/measure_group").toInt();
    ConnectorSdo::SetServer(strSdo, nGroup);
}

void SdoWorker::CheckInstruct(QString strEkasuiServer)
{
    QString execString = QString("SELECT i.jp_instructions_id, i.sdo_mmember_id, i.sdo_status_id, i.sdo_time_begin, i.sdo_time_begin_server, i.jp_id, l.sdo_content_designer, i.\"date\" "
                                 "FROM a_sync_instruct i "
                                 "inner join sync_l_ekasui_2_instruct l on i.jp_id = l.jp_id "
                                 //"WHERE date = cast('%1' as date) and sdo_mmember_id is not null and sdo_evaluation is null")
                                 "WHERE sdo_mmember_id is not null and sdo_evaluation is null");
            //.arg(m_currentDay.toString("yyyy-MM-dd"));
    QSqlQuery qr(m_database);
    if(!qr.exec(execString))
    {
        m_database.writeLog(QString("Не удалось выполнить запрос %1").arg(execString), dBase::EKASUI);
        return;
    }

    while(qr.next() && !m_bStopping.load())
    {
        int jp_instructions_id = qr.value(0).toInt();
        int sdo_mmember_id = qr.value(1).toInt();
        int sdo_status_id = qr.value(2).toInt();
        auto sdo_time_begin = qr.value(3).toDateTime();
        auto sdo_time_begin_server = qr.value(4).toDateTime();
        //int jp_id = qr.value(5).toInt();
        int contentDesigner = qr.value(6).toInt();
        QDate dtDate = qr.value(7).toDate();

        bool bExpired(false);
        bool bNeedSend2Sdo(false);

        ConnectorSdo::memberResult result;
        QDateTime tmCurrent = QDateTime::currentDateTime();
        if (ConnectorSdo::GetMemberResult(sdo_mmember_id, result))
        {

            sdo_time_begin = result.m_tmSdoTimeStart;
            if (sdo_time_begin_server.isNull())
                sdo_time_begin_server = tmCurrent;

            QDateTime tmStart = sdo_time_begin.isNull() ? sdo_time_begin_server : sdo_time_begin;
            bExpired = (!tmStart.isNull() && tmStart.secsTo(tmCurrent) > 20 * 60) ||
                    dtDate < tmCurrent.date();

            if (InstructWorker::SdoStatus_Passed == result.m_shSdoStatusId && result.m_shSdoStatusId != sdo_status_id)
            {
                // Статус изменился на Пройден с любого другого.
                // Это говорит о том, что инструктаж закончен, а вот завершить его с оценкой силами СДО не получается
                switch (contentDesigner)
                {
                    case 1: // ОНИЛ
                    {
                        result.m_shSdoStatusId = result.m_prSdoEvaluation >= 90 ? InstructWorker::SdoStatus_Passed : InstructWorker::SdoStatus_NotPassed;
                        bNeedSend2Sdo = true;
                        break;
                    }
                    case 2: // НовАТранс
                    {
                        result.m_shSdoStatusId = result.m_prSdoEvaluation == 100 ? InstructWorker::SdoStatus_Passed : InstructWorker::SdoStatus_NotPassed;
                        bNeedSend2Sdo = true;
                        break;
                    }
                }
            }
            else if (bExpired)
            {
                result.m_shSdoStatusId = InstructWorker::SdoStatus_NotPassed;
                bNeedSend2Sdo = true;
            }
        }
        else
        {
            if (dtDate < tmCurrent.date())
            {
                // Инструктаж не начинался и уже просрочен
                result.m_shSdoStatusId = InstructWorker::SdoStatus_NotPassed;
                bNeedSend2Sdo = true;
                bExpired = true;
            }
        }

        if(result.m_shSdoStatusId != sdo_status_id)
        {
            bool bSent(false), bNewEkasuiStatus(false);
            if (InstructWorker::SdoStatus_NotPassed == result.m_shSdoStatusId || InstructWorker::SdoStatus_Passed == result.m_shSdoStatusId)
            {
                bSent = m_instruct.IsEnabled() && InstructWorker::SendEkasuiResult(m_database, strEkasuiServer, jp_instructions_id, InstructWorker::EkasuiStatus_InstructExecuted, result.m_shSdoStatusId);
                bNewEkasuiStatus = true;
            }

            if (bNeedSend2Sdo)
            {
                QSqlQuery upd(m_database);
                execString = QString("update a_sync_instruct set sdo_status_id = %1, sdo_evaluation = %2, sdo_points = %3, sdo_duration = %4, sdo_time_end = %5 where jp_instructions_id = %6")
                        .arg(result.m_shSdoStatusId)
                        .arg(result.m_prSdoEvaluation)
                        .arg(result.m_dfSdoPoints)
                        .arg(result.m_nSdoDuration)
                        .arg(result.m_tmSdoTimeEnd.isValid() ? result.m_tmSdoTimeEnd.toString("''yyyy-MM-dd hh:mm:ss''") : "NULL")
                        .arg(QString::number(jp_instructions_id));
                if(!upd.exec(execString))
                    m_database.writeLog(QString("Не удалось выполнить запрос %1").arg(execString), dBase::EKASUI);

                if (bNewEkasuiStatus)
                {
                    execString = QString("INSERT INTO a_sync_instruct_ekasui_status (jp_instructions_id, status, date_set, date_sent, \"index\") select %1, %2, '%3', %4, coalesce(max(\"index\"), -1) + 1 from a_sync_instruct_ekasui_status where jp_instructions_id = %5")
                            .arg(QString::number(jp_instructions_id))
                            .arg(result.m_shSdoStatusId)
                            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                            .arg(bSent ? QDateTime::currentDateTime().toString("''yyyy-MM-dd hh:mm:ss''") : "NULL")
                            .arg(QString::number(jp_instructions_id));
                    if(!upd.exec(execString))
                        m_database.writeLog(QString("Не удалось выполнить запрос %1").arg(execString), dBase::EKASUI);
                }
            }
        }
    }
}

void SdoWorker::process()
{
    QString strError;
    if (m_database.Connect(strError))
        m_database.writeLog("Старт работы с СДО", dBase::SDO);
    else
        qDebug() << strError;

    while (!m_bStopping.load())
    {
        XmlSettings xmlSet;
        if (    m_database.Connect(strError) &&
                xmlSet.Load(m_database))
            CheckInstruct(xmlSet.GetValue("ekasui/server"));

        // Теперь можно и поспать

        QDateTime tmCur = QDateTime::currentDateTime();
        QDateTime tmWakeup = std::min(QDateTime(tmCur.date().addDays(1), QTime(0, 0)), tmCur.addSecs(10 * 60));

        while (!m_bStopping.load() && tmCur < tmWakeup)
        {
            QThread::sleep(static_cast<unsigned long>(std::min(10, int(tmCur.secsTo(tmWakeup)))));
            tmCur = QDateTime::currentDateTime();
        }
    }

    if (m_database.Connect(strError))
        m_database.writeLog("Завершение работы с СДО", dBase::SDO);
    else
        qDebug() << strError;

    m_database.Close();

    emit(finished());
}

void SdoWorker::stop()
{
    m_bStopping = true;
}
