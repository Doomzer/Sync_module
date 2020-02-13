#include <QDebug>
#include <QThread>
//#include <QDateTime>
//#include <QSqlQuery>
#include <QSqlError>


#include "Ekasui/JpiServiceSOAP.nsmap"

#include "instructworker.h"
#include "ekasuiinstruct.h"
#include "connectorsdo.h"
#include "xmlsettings.h"
#include "Ekasui/soapJpiServiceSOAPProxy.h"

InstructWorker::InstructWorker(const EkasuiInstruct &instruct, const dBase &db, QList<unsigned> &listDor, int timeZone, QObject *parent)
    : QObject(parent)
    , m_listDor(listDor)
    , m_timeZone(timeZone)
    , m_database(db)
    , m_bStopping(false)
    , m_instruct(instruct)
{
}

void InstructWorker::process()
{
    QString strDor, error;
    foreach (const unsigned id, m_listDor)
    {
        if (strDor.length() > 0)
            strDor += ", ";
        strDor += QString::number(id);
    }

    QString str = QString("Запуск потока обработки работ ЕКАСУИ по дорогам: %1 (%2)")
            .arg(strDor)
            .arg(QString::number(int64_t(QThread::currentThreadId())));
    if (m_database.Connect(error))
        m_database.writeLog(str, dBase::EKASUI);
    else
        qDebug() << str;

    m_bStopping = false;

    QDateTime tmNextEkasuiLoad;

    while(!m_bStopping.load())
    {
        QDateTime tmCur = QDateTime::currentDateTime();

        // Проверим, надо ли запрашивать информацию у ЕКАСУИ
        if (tmNextEkasuiLoad.isNull() ||
            tmNextEkasuiLoad <= tmCur)
        {
            if (m_database.Connect(error))
            {
                QDate dtCur = tmCur.date();
                ProcessEkasui(dtCur);

                if (tmCur.time().hour() >= 20)
                {
                    // Начиная с 20:00 еще будем обрабатывать завтрашние работы
                    dtCur = dtCur.addDays(1);
                    ProcessEkasui(dtCur);
                }

                // Следующий раз работать будем в начале следующего часа
                if (tmNextEkasuiLoad.isNull())
                    tmNextEkasuiLoad = tmCur;
                tmNextEkasuiLoad = tmNextEkasuiLoad.addSecs(60 * 60);
                QTime tm = tmNextEkasuiLoad.time();
                tm.setHMS(tm.hour(), 0, 0);
                tmNextEkasuiLoad.setTime(tm);

                // Если обработка заняла больше часа - не страшно, запустим ее сразу еще раз
                /*while (tmNextEkasuiLoad < QDateTime::currentDateTime())
                {
                    // Это если вдруг обработка заняла больше часа
                    tmNextEkasuiLoad = tmNextEkasuiLoad.addSecs(60 * 60);
                }*/
            }
            else
                tmNextEkasuiLoad = tmCur.addSecs(5 * 60); // Коннект к базе не состоялся, попробуем через 5 минут
        }

        // статус назначенных инструктажей проверяется в отдельном потоке
        QDateTime tmWakeup = tmNextEkasuiLoad;
        /*if(!m_bStopping.load() && ConnectorSdo::IsEnabled())
        {
            XmlSettings xmlSet;
            if (m_database.Connect(error) &&
                xmlSet.Load(m_database) &&
                CheckInstruct(xmlSet.GetValue("ekasui/server")))
            {
                // Инструктажи закончились, можно и поспать подольше
            }
            else
                tmWakeup = std::min(tmWakeup, QDateTime::currentDateTime().addSecs(10 * 60)); // Проснемся через 10 минут
        }*/

        m_database.Close();

        tmCur = QDateTime::currentDateTime();
        while (!m_bStopping.load() && tmCur < tmWakeup)
        {
            QThread::sleep(static_cast<unsigned long>(std::min(10, int(tmCur.secsTo(tmWakeup)))));
            tmCur = QDateTime::currentDateTime();
        }
    }

    str = QString("Завершение потока обработки работ ЕКАСУИ по дорогам: %1 (%2)")
            .arg(strDor)
            .arg(QString::number(int64_t(QThread::currentThreadId())));
    if (m_database.Connect(error))
        m_database.writeLog(str, dBase::EKASUI);
    else
        qDebug() << str;

    emit finished();

    return;
}

void InstructWorker::stop()
{
    m_bStopping = true;
}

void InstructWorker::ProcessEkasui(QDate dtCurDay)
{
    std::vector<std::string> prPred;
    std::vector<std::string> prPodr;

    QString strFilter;
    foreach (unsigned dorKod, m_listDor) {
        if (strFilter.length() > 0)
            strFilter += ", ";
        strFilter += QString::number(dorKod);
    }
    //QString::number((int64_t)QThread::currentThreadId())
    XmlSettings xmlSett;
    xmlSett.Load(m_database);
    QString strEkasuiServer = xmlSett.GetValue("ekasui/server");

    m_database.writeLog(QString("Старт запроса данных от ЕКАСУ-И (ЕКАСУИ: %1; дороги: %2)").arg(strEkasuiServer, strFilter), dBase::EKASUI);

    QString execString = QString("SELECT PO.Pred_id, PO.Podr_Id FROM sync_podr PO inner join sync_pred PE on PO.Pred_id = PE.Pred_id  WHERE PO.deleted is null and PE.Vd_Id between 900 and 999 and PE.Gr_Id = 70 and PE.dor_kod in (%1)")
            .arg(strFilter);

    QSqlQuery qr(m_database);
    if(!qr.exec(execString))
    {
        m_database.writeLog(QString("Не удалось выполнить запрос %1").arg(execString), dBase::EKASUI);
        return;
    }
    else
    {
        while (qr.next())
        {
            prPred.push_back(qr.value(0).toString().toStdString());
            prPodr.push_back(qr.value(1).toString().toStdString());
        }
    }

    m_database.writeLog(QString("Дороги: %1. Количество бригад для запроса: %2").arg(strFilter).arg(prPred.size()), dBase::EKASUI);
    /*if (!qr.exec("notify docform_update;"))
        qDebug() << "Ошибка отправки notify docform_update";*/

    QString msgStr;
    int JPI_size = 0;
    try
    {
        JpiServiceSOAPProxy proxy(SOAP_C_UTFSTRING);
        proxy.soap->recv_timeout = 5;
        proxy.soap->connect_timeout = 10;
        int nRes(SOAP_OK);

        for (unsigned i = 0; !m_bStopping.load() && i < prPodr.size(); ++i)
        {
            _ns1__GetJpiRequest rq;
            _ns1__GetJpiResponse rr;
            rq.Date = dtCurDay.toString("dd.MM.yyyy").toStdString();
            rq.Pred = "PRED_" + prPred[i];
            rq.Podr = "PODR_" + prPred[i] + "_" + prPodr[i];
            QString serverGet = strEkasuiServer + "/getjpi";
            nRes = proxy.GetJPI(serverGet.toStdString().c_str(), "GetJPI", &rq, rr);
            if (SOAP_OK == nRes)
            {
                bool bEkasui = m_instruct.IsEnabled();
                JPI_size += rr.JPI ? rr.JPI->size() : 0;
                for (unsigned j = 0; nullptr != rr.JPI && j < rr.JPI->size(); ++j)
                {
                    _ns1__GetJpiResponse_JPI &resp = rr.JPI->at(j);
                    ProcessJPI(bEkasui, strEkasuiServer, dtCurDay, std::stoi(prPred[i]), std::stoi(prPodr[i]), resp);
                    //updateFromE(std::stoi(prPred[i]), std::stoi(prPodr[i]), resp, qr, strServer);
                }
            }
            else
            {
                msgStr = "Дороги: %1. Ошибка при получении работ по бригаде (%2. Ошибка: %3)";
                m_database.writeLog(msgStr.arg(strFilter).arg(QString::fromStdString(rq.Podr), QString::number(nRes)), dBase::EKASUI);
            }
        }
        soap_end(proxy.soap);
    }
    catch(...)
    {
        m_database.writeLog(QString("Дороги: %1. Неизвестная ошибка при синхронизации ЕКАСУИ").arg(strFilter), dBase::EKASUI);
    }

    m_database.writeLog(QString("Дороги: %1. Синхронизация ЕКАСУИ завершена. Всего получено %2 работ").arg(strFilter).arg(JPI_size), dBase::EKASUI);
}

void InstructWorker::ProcessJPI(bool bSendResult, QString strServer, QDate dtCurDay, int predId, int podrId, _ns1__GetJpiResponse_JPI &resp)
{
    // Найдем работника
    std::pair<int, int> prsId = GetPersonalId(predId, podrId, resp, true);
    if (0 == prsId.first)
        return;

    // Найдем jp_id
    QString strName;
    std::pair<int, int> jpId = GetJP(resp, strName, true);
    if (0 == jpId.first)
        return;

    int meId(0), mmId(0);
    if(0 != jpId.second)
    {
        // По данной работе есть инструктаж
        QString strMName = QString("%1. Инструктаж. %2").arg(dtCurDay.toString("yyyy.MM.dd")).arg(strName).left(255);
        meId = GetMeasure(jpId.first, dtCurDay, jpId.second, strMName, true);
        if (0 != meId)
            mmId = ConnectorSdo::MemberToMeasure(meId, prsId.second);
    }

    // Запишем строчку в a_sync_instruct
    QString execString = QString("INSERT INTO a_sync_instruct (jp_instructions_id, jp_id, personal_id, sdo_mmember_id, date) "
                                 "select %1, %2, %3, %4, '%5' where not exists "
                                 "(select 1 from a_sync_instruct where jp_instructions_id = %1)")
            .arg(QString::number(resp.jpinstructionsid))
            .arg(QString::number(jpId.first))
            .arg(QString::number(prsId.first))
            .arg(0 == mmId ? "null" : QString::number(mmId))
            .arg(dtCurDay.toString("yyyy-MM-dd"));
    QSqlQuery qr(m_database);
    if(qr.exec(execString))
    {
        //m_database.writeLog(QString("Добавили новую запись в a_sync_instruct с jp_instructions_id = %1").arg(QString::number(resp.jpinstructionsid)), dBase::EKASUI);

        QString msgStr = "";
        int status(0 == mmId ? EkasuiStatus_InstructNone : EkasuiStatus_InstructAssigned);
        bool bSent = bSendResult && SendEkasuiResult(m_database, strServer, resp.jpinstructionsid, status, SdoStatus_NotStarted);

        execString = QString("INSERT INTO a_sync_instruct_ekasui_status (jp_instructions_id, status, date_set, date_sent, \"index\") select %1, %2, '%3', %4, coalesce(max(\"index\"), -1) + 1 from a_sync_instruct_ekasui_status where jp_instructions_id = %5")
                .arg(QString::number(resp.jpinstructionsid))
                .arg(QString::number(status))
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                .arg(bSent ? QDateTime::currentDateTime().toString("''yyyy-MM-dd hh:mm:ss''") : "null")
                .arg(QString::number(resp.jpinstructionsid));
        if(!qr.exec(execString))
        {
            QString strError = qr.lastError().text();
            m_database.writeLog(QString("Не удалось выполнить запрос вставки новой записи в a_sync_instruct_ekasui_status с jp_instructions_id = %1").
                                arg(QString::number(resp.jpinstructionsid)), dBase::EKASUI);
            m_database.writeLog(QString("Ошибка: %1").arg(strError), dBase::EKASUI);
        }
    }
    else
        m_database.writeLog(QString("Не удалось выполнить запрос вставки новой записи в a_sync_instruct с jp_instructions_id = %1").arg(QString::number(resp.jpinstructionsid)), dBase::EKASUI);
}

std::pair<int, int> InstructWorker::GetJP(_ns1__GetJpiResponse_JPI &resp, QString &strName, bool bCreate)
{
    std::pair<int, int> out(0, 0);
    QSqlQuery qr(m_database);
    QString execString = QString("SELECT jp_id, name, coalesce(sdo_content_id, 0) FROM sync_l_ekasui_2_instruct where jpnum = '%1'")
            .arg(QString::fromStdString(resp.jpnum));
    if(qr.exec(execString))
    {
        if (qr.next())
        {
            out.first = qr.value(0).toInt();
            strName = qr.value(1).toString();
            out.second = qr.value(2).toInt();
            /*m_database.writeLog(QString("По jpnum = %1 нашли работу (jp_id: %2, sdo_id: %3)")
                                .arg(QString::fromStdString(resp.jpnum))
                                .arg(QString::number(out.first))
                                .arg(QString::number(out.second)), dBase::EKASUI);*/
        }
        else if (bCreate)
        {
            // Надо создать новую запись
            //m_database.writeLog(QString("Создаем новую работу в sync_l_ekasui_2_instruct с jpnum = %1").arg(QString::fromStdString(resp.jpnum)), dBase::EKASUI);
            execString = QString("insert into sync_l_ekasui_2_instruct (jp_id, jpnum, name, description) select coalesce(max(jp_id), 0) + 1, '%1', '', '' from sync_l_ekasui_2_instruct")
                    .arg(QString::fromStdString(resp.jpnum));
            if (qr.exec(execString))
            {
                return GetJP(resp, strName, false);
            }
            else
                m_database.writeLog(QString("Не удалось выполнить запрос %1").arg(execString), dBase::EKASUI);
        }
    }
    else
        //m_database.writeLog(QString("Не удалось выполнить запрос %1").arg(execString), dBase::EKASUI);
        m_database.writeLog(QString("Не удалось начитать работу в sync_l_ekasui_2_instruct по jpnum = %1").arg(QString::fromStdString(resp.jpnum)), dBase::EKASUI);

    return out;
}

bool InstructWorker::SendEkasuiResult(dBase &database, QString strServer, int jpInstrId, int statusEkasui, int statusSdo)
{
    JpiServiceSOAPProxy proxy(SOAP_C_UTFSTRING);

    _ns1__SetJpiRequest rq;
    _ns1__SetJpiRequest_JPI val;
    _ns1__SetJpiResponse rr;
    val.jpinstructionsid = jpInstrId;
    val.status = std::to_string(statusEkasui);
    if(EkasuiStatus_InstructExecuted == statusEkasui)
        val.dateresult = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss").toStdString();

    if (SdoStatus_Passed == statusSdo)
        val.result = "1";
    else if (SdoStatus_NotPassed == statusSdo)
        val.result = "-1";

    rq.JPI = SOAP_NEW(std::vector<_ns1__SetJpiRequest_JPI>);
    rq.JPI->push_back(val);

    proxy.soap->recv_timeout = 5;
    proxy.soap->connect_timeout = 10;
    QString server = strServer + "/setjpi";
    int nRes = proxy.SetJPI(server.toStdString().c_str(), "SetJPI", &rq, rr);

    QString msgStr = "";
    //if(SOAP_OK != nRes)
    {
        msgStr = "CasrvInstructProcessor::SendResult(%1). Отправка результата в ЕКАСУ-И (IstrId=%2, Status=%3, Result=\"%4\")";
        if(SOAP_OK != nRes)
            msgStr = msgStr.arg("Ошибка: " + QString::number(nRes));
        else
            msgStr = msgStr.arg("Ok");
        msgStr = msgStr.arg(val.jpinstructionsid);
        msgStr = msgStr.arg(QString::fromStdString(val.status));
        msgStr = msgStr.arg(QString::fromStdString(val.result));
        database.writeLog(msgStr, dBase::EKASUI);
    }

    soap_end(proxy.soap);

    return SOAP_OK == nRes;
}

std::pair<int, int> InstructWorker::GetPersonalId(int predId, int podrId, _ns1__GetJpiResponse_JPI &resp, bool bCreate)
{
    std::pair<int, int> out;

    QString execString = QString("SELECT personal_id FROM sync_instruct_personal where person_id = '%1'").arg(QString::fromStdString(resp.personid));
    QSqlQuery qr(m_database);
    if(qr.exec(execString))
    {
        if (qr.first())
        {
            out.first = qr.value(0).toInt();
            if (0 != out.first)
            {
                // Найдем SdoId
                const auto &pos = m_mapPerson2Sdo.find(out.first);
                if (m_mapPerson2Sdo.end() == pos)
                {
                    // Запросим его у СДО
                    QString strError;
                    std::vector<QString> pers = ConnectorSdo::GetPerson(QString::fromStdString(resp.tpperson), strError);
                    if (0 == pers.size())
                        m_database.writeLog(QString("Не удалось получить информацию по пользователю (tpperson: %1): %2").arg(QString::fromStdString(resp.tpperson)).arg(strError), dBase::SDO);
                    else if (pers.size() < 1 ||
                        pers[0] == "")
                        m_database.writeLog(QString("Не удалось получить email (tpperson: %1)").arg(QString::fromStdString(resp.tpperson)), dBase::SDO);
                    else if(pers.size() < 2 ||
                       pers[1] == "")
                    {
                        m_database.writeLog(QString("Не удалось получить personid (tpperson: %1)").arg(QString::fromStdString(resp.tpperson)), dBase::SDO);
                    }
                    else
                    {
                        out.second = pers[1].toInt();
                        if (0 != out.second)
                        {
                            m_mapPerson2Sdo[out.first] = out.second;

                            // Обновим email в таблице сотрудников
                            execString = QString("UPDATE sync_personal set tpperson = %1, email = '%2', modified = '%4' where (personid = '%3' or "
                                                 "pred_id = %5 and podr_id = %6 and (\"name\" || ' ' || first_name || ' ' || last_name) ilike '%7') and deleted id null")
                                    .arg(QString::fromStdString(resp.tpperson))
                                    .arg(pers[0])
                                    .arg(QString::fromStdString(resp.personid))
                                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                    .arg(QString::number(predId))
                                    .arg(QString::number(podrId))
                                    .arg(QString::fromStdString(resp.displayname));
                            if(!qr.exec(execString))
                                m_database.writeLog(QString("Не удалось выполнить запрос %1").arg(execString), dBase::EKASUI);
                            // Обновим email также и в таблице sync_personal
                        }
                    }
                }
                else
                    out.second = pos->second;
            }
        }
        else if (bCreate)
        {
            // Создаем нового сотрудника
            //m_database.writeLog(QString("Пытаемся записать нового сотрудника: %1").arg(QString::fromStdString(resp.personid)), dBase::EKASUI);
            execString = QString("INSERT INTO sync_instruct_personal (personal_id, pred_id, podr_id, fio, person_id, tpperson) select coalesce(max(personal_id), 0) + 1, %1, %2, '%3', '%4', '%5' from sync_instruct_personal")
                    .arg(QString::number(predId))
                    .arg(QString::number(podrId))
                    .arg(QString::fromStdString(resp.displayname))
                    .arg(QString::fromStdString(resp.personid))
                    .arg(QString::fromStdString(resp.tpperson));
            //m_database.writeLog(QString("Записываем сотрудника: %1").arg(execString), dBase::EKASUI);
            if(qr.exec(execString))
                return GetPersonalId(predId, podrId, resp, false);
            else
                m_database.writeLog(QString("Не удалось выполнить запрос %1").arg(execString), dBase::EKASUI);
        }
    }
    else
        m_database.writeLog(QString("Не удалось выполнить запрос %1").arg(execString), dBase::EKASUI);

    return out;
}

int InstructWorker::GetMeasure(int jpId, QDate dtCurDay, int contentId, QString strName, bool bCreate)
{
    QString execString = QString("SELECT sdo_measure_id FROM a_sync_l_instruct_2_measure where jp_id = %1 and date = cast('%2' as date)")
            .arg(QString::number(jpId))
            .arg(dtCurDay.toString("yyyy-MM-dd"));
    QSqlQuery qr(m_database);
    int mId(0);
    if(qr.exec(execString))
    {
        if(qr.next())
            mId = qr.value(0).toInt();
        else if (bCreate)
        {
            auto tmBegin = QDateTime::currentDateTime();
            QDateTime tmEnd(dtCurDay, QTime(23, 59, 59));
            // Добавлять время не будем, даже если на выполнение останется меньше 20 минут
            // Ибо нефик добавлять работы на сегодня перед полуночью
            /*if (tmBegin.addSecs(20 * 60) > tmEnd) // 20 минут
            {
                tmEnd = tmEnd.addSecs(6 * 60 * 60 - 1); // 5:59:59;
                if (tmEnd > tmBegin.addSecs(2 * 60 * 60)) // 2 часа
                    tmEnd = tmBegin.addSecs(2 * 60 * 60);
            }*/
            int tmpId = ConnectorSdo::CreateMeasure(contentId, strName, tmBegin, tmEnd);
            if (0 != tmpId)
            {
                execString = QString("INSERT INTO a_sync_l_instruct_2_measure (jp_id, sdo_measure_id, aos_schedule, date_end, date) VALUES (%1, %2, NULL, '%4', '%5')")
                        .arg(QString::number(jpId))
                        .arg(QString::number(mId))
                        .arg(tmEnd.toString("yyyy-MM-dd hh:mm:ss"))
                        .arg(dtCurDay.toString("yyyy-MM-dd"));
                if(qr.exec(execString))
                    mId = GetMeasure(jpId, dtCurDay, contentId, strName, false);
                else
                    m_database.writeLog(QString("Не удалось выполнить запрос %1").arg(execString), dBase::EKASUI);
            }
        }
    }
    else
        m_database.writeLog(QString("Не удалось выполнить запрос %1").arg(execString), dBase::EKASUI);

    return mId;
}

/*bool InstructWorker::CheckInstruct(QString strEkasuiServer)
{
    // Надо будет завершать все инструктажи за прошлые дни

    // возвращаем true, если выполнены все назначенные инструктажи
    // в противном случае возвращается false

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
        return false;
    }
    if(qr.size() == 0)
        return true;

    while(qr.next())
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

            if (SdoStatus_Passed == result.m_shSdoStatusId && result.m_shSdoStatusId != sdo_status_id)
            {
                // Статус изменился на Пройден с любого другого.
                // Это говорит о том, что инструктаж закончен, а вот завершить его с оценкой силами СДО не получается
                switch (contentDesigner)
                {
                    case 1: // ОНИЛ
                    {
                        result.m_shSdoStatusId = result.m_prSdoEvaluation >= 90 ? SdoStatus_Passed : SdoStatus_NotPassed;
                        bNeedSend2Sdo = true;
                        break;
                    }
                    case 2: // НовАТранс
                    {
                        result.m_shSdoStatusId = result.m_prSdoEvaluation == 100 ? SdoStatus_Passed : SdoStatus_NotPassed;
                        bNeedSend2Sdo = true;
                        break;
                    }
                }
            }
            else if (bExpired)
            {
                result.m_shSdoStatusId = SdoStatus_NotPassed;
                bNeedSend2Sdo = true;
            }
        }
        else
        {
            if (dtDate < tmCurrent.date())
            {
                // Инструктаж не начинался и уже просрочен
                result.m_shSdoStatusId = SdoStatus_NotPassed;
                bNeedSend2Sdo = true;
                bExpired = true;
            }
        }

        if(result.m_shSdoStatusId != sdo_status_id)
        {
            bool bSent(false), bNewEkasuiStatus(false);
            if (SdoStatus_NotPassed == result.m_shSdoStatusId || SdoStatus_Passed == result.m_shSdoStatusId)
            {
                bSent = SendEkasuiResult( strEkasuiServer, jp_instructions_id, EkasuiStatus_InstructExecuted, result.m_shSdoStatusId);
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

    return false;
}*/

