#include <QDebug>
#include <QThread>
#include <QDomDocument>

#include "asuinstruct.h"
#include "ekasuiinstruct.h"
#include "xmlsettings.h"
#include "net.h"

AsuInstruct::AsuInstruct(const dBase &db, QObject *parent)
    : QObject(parent)
    , m_dbSettings(db)
    , m_bStopping(false)
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

AsuInstruct::~AsuInstruct()
{
    qDebug() << "AsuInstruct::~AsuInstruct()";
}

QString AsuInstruct::GetSid(QString login, QString password, int predId, int sotrId)
{
    //QString url = "http://10.248.33.110/assh2/rc/kzrc.dll/logon?login=%1&password=%2&pred_id=%3&newlogin=%4";
    QString url = "http://bigbro/assh2/rc/kzrc.dll/logon?login=%1&password=%2&pred_id=%3&newlogin=%4";
    url = url.arg(login, password, QString::number(predId), QString::number(sotrId));
    Net handler;
    auto temp = handler.CheckSite(url, GET);

    QDomDocument doc;
    doc.setContent(temp);
    QDomElement root = doc.documentElement();
    return root.attribute("sid");
}

std::vector<std::vector<QString>> AsuInstruct::getTable(const QString &strSrv, QString sid, QString formID)
{
    //QString url = "http://10.248.33.110/assh2/UO/Onil.Web.Uo/Default.aspx/GetFormData?sid=%1";
    //QString url = "http://bigbro/assh2/UO/Onil.Web.Uo/Default.aspx/GetFormData?sid=%1";
    QString url = QString("http://%1/assh2/UO/Onil.Web.Uo/Default.aspx/GetFormData?sid=%2").arg(strSrv);
    std::vector<std::vector<QString>> result;
    url = url.arg(sid);
    Net handler;
    auto temp = handler.CheckSite(url, formID, POST);
    temp = temp.replace("\\u","$$$u");
    temp = temp.replace("\\","");
    temp = temp.replace("$$$u","\\u");
    temp = temp.replace("\"{","{");
    temp = temp.replace("}\"","}");
    QJsonDocument document = QJsonDocument::fromJson(temp);
    if (document.isObject())
    {
        QJsonObject tempObj = document.object();
        QJsonArray tempArray;
        foreach(const QString& key, tempObj.keys())
        {
            tempArray = tempObj.value(key).toArray();
        }
        foreach (const QJsonValue & value, tempArray)
        {
            QJsonObject tempObj2 = value.toObject();
            std::vector<QString> tempVect;
            if(formID == "21530903")
            {
                tempVect.push_back(tempObj2.value("dor_kod").toString());
                tempVect.push_back(tempObj2.value("Name").toString());
                tempVect.push_back(tempObj2.value("Sname").toString());
            }
            else if(formID == "21530901")
            {
                tempVect.push_back(tempObj2.value("Dol_id").toString());
                tempVect.push_back(tempObj2.value("Name").toString());
                tempVect.push_back(tempObj2.value("SName").toString());
                tempVect.push_back(tempObj2.value("VName").toString());
                tempVect.push_back(tempObj2.value("Cor_Tip").toString());
            }
            else if(formID == "21530904")
            {
                tempVect.push_back(tempObj2.value("Pred_id").toString());
                tempVect.push_back(tempObj2.value("Dor_Kod").toString());
                tempVect.push_back(tempObj2.value("Vd_Id").toString());
                tempVect.push_back(tempObj2.value("Name").toString());
                tempVect.push_back(tempObj2.value("SName").toString());
                tempVect.push_back(tempObj2.value("VName").toString());
                tempVect.push_back(tempObj2.value("GR_id").toString());
                tempVect.push_back(tempObj2.value("Cor_Tip").toString());
            }
            else if(formID == "21530905")
            {
                tempVect.push_back(tempObj2.value("Pred_id").toString());
                tempVect.push_back(tempObj2.value("Podr_Id").toString());
                tempVect.push_back(tempObj2.value("Podr_Nom").toString());
                tempVect.push_back(tempObj2.value("Name").toString());
                tempVect.push_back(tempObj2.value("SName").toString());
                tempVect.push_back(tempObj2.value("VName").toString());
                tempVect.push_back(tempObj2.value("Cor_Tip").toString());
            }
            else if(formID == "21530902")
            {
                tempVect.push_back(tempObj2.value("Pred_id").toString());
                tempVect.push_back(tempObj2.value("Sotr_Id").toString());
                tempVect.push_back(tempObj2.value("Podr_id").toString());
                tempVect.push_back(tempObj2.value("Dol_Id").toString());
                tempVect.push_back(tempObj2.value("Name").toString());
                tempVect.push_back(tempObj2.value("First_Name").toString());
                tempVect.push_back(tempObj2.value("Last_Name").toString());
                tempVect.push_back(tempObj2.value("PERSONID").toString());
                tempVect.push_back(tempObj2.value("Cor_Tip").toString());
            }
            else if(formID == "21530907")
            {
                tempVect.push_back(tempObj2.value("pred_v_id").toString());
                tempVect.push_back(tempObj2.value("pred_n_id").toString());
            }
            result.push_back(tempVect);
        }

    }
    return result;
}

QString AsuInstruct::tran(QString text)
{
    QString rus[70]={"А", "а", "Б", "б", "В", "в", "Г", "г", "Ґ", "ґ", "Д", "д", "Е", "е", "Є", "є", "Ж", "ж", "З", "з", "И", "и", "Й", "й", "К", "к",
     "Л", "л", "М", "м", "Н", "н", "О", "о", "П", "п",   "Р", "р", "С", "с", "Т", "т", "У", "у", "Ф", "ф", "Х", "х", "Ц", "ц", "Ч", "ч", "Ш", "ш",
     "Щ", "щ", "Ь", "ь", "Ю", "ю", "Я", "я", "Ы", "ы", "Ъ", "ъ", "Ё", "ё", "Э", "э"};

     QString eng[70]={"A", "a", "B", "b", "V", "v", "G", "g", "G", "g", "D", "d", "E", "e", "E", "E", "Zh", "zh", "Z", "z", "I", "i", "J", "j", "K", "k",
     "L", "l", "M", "m", "N", "n", "O", "o", "P", "p",   "R", "r", "S", "s", "T", "t", "U", "u", "F", "f", "H", "h", "Ts", "ts", "ch", "ch", "Sh", "sh",
     "Shh", "shh", "", "", "Yu", "yu","Ya", "ya", "Y", "y", "", "", "Yo", "yo", "E", "e"};

     bool find=false;
     QString ret;
     for(int i=0; i <= text.length(); i++)
     {
       find=false;
       for(int j=0; j<70; j++)
       {
           if( text.mid(i,1).compare(rus[j])==0 )
           {
               ret+=eng[j];
               find=true;
               break;
           }
       }
       if( ! find )
           ret+=text.mid(i,1);
     }
     return ret;
}

QString AsuInstruct::updateDB(const QString &strSrv, dbName name)
{
    unsigned arg = 0;
    long countAll = 0, countIn = 0, countUp = 0;
    QString firstSelectT;
    QString secondSelectT;
    QString thirdSelectT;
    QString firstUpdateT;
    QString firstUpdateTn;
    QString firstInsertT;
    QString firstInsertTn;
    QString dbTab = "";
    QString formID = "";

    switch (name)
    {
        case DOR:
        {
            arg = 3;
            firstSelectT = "SELECT Dor_Kod, Name, Sname FROM DOR";
            secondSelectT = "SELECT * FROM sync_dor WHERE dor_kod = %1 and name = '%2' and sname = '%3'";
            thirdSelectT = "SELECT * FROM sync_dor WHERE dor_kod = %1";
            firstUpdateT = "UPDATE sync_dor SET name = '%2', sname = '%3' WHERE dor_kod = %1";
            firstInsertT = "INSERT INTO sync_dor (dor_kod, name, sname) VALUES (%1, '%2', '%3')";
            dbTab = "DOR";
            formID = "21530903";
        }
        break;

        case DOL:
        {
            arg = 5;
            firstSelectT = "SELECT Dol_id, Name, Sname, Vname, Cor_Tip FROM DOL";
            secondSelectT = "SELECT * FROM sync_dol WHERE dol_id = %1 and name = '%2' and sname = '%3' and vname = '%4'";
            thirdSelectT = "SELECT * FROM sync_dol WHERE dol_id = %1";
            firstUpdateT = "UPDATE sync_dol SET name = '%2', sname = '%3', vname = '%4', deleted = '%5', modified = '%6' WHERE dol_id = %1";
            firstUpdateTn = "UPDATE sync_dol SET name = '%2', sname = '%3', vname = '%4', deleted = %5, modified = '%6' WHERE dol_id = %1";
            firstInsertT = "INSERT INTO sync_dol (dol_id, name, sname, vname, deleted, added) VALUES (%1, '%2', '%3', '%4', '%5', '%6')";
            firstInsertTn = "INSERT INTO sync_dol (dol_id, name, sname, vname, deleted, added) VALUES (%1, '%2', '%3', '%4', %5, '%6')";
            dbTab = "DOL";
            formID = "21530901";
        }
        break;

        case PRED:
        {
            arg = 8;
            firstSelectT = "SELECT Pred_id, Dor_Kod, Vd_Id, Name, Sname, Vname, GR_id, Cor_Tip FROM PRED WHERE (Vd_Id between 900 and 999 and Gr_Id in (7,70)) or (Vd_Id = 0 and gr_id = 10)";
            secondSelectT = "SELECT * FROM sync_pred WHERE pred_id = %1 and dor_kod = %2 and vd_id = %3 and name = '%4' and sname = '%5' and vname = '%6' and gr_id = '%7'";
            thirdSelectT = "SELECT * FROM sync_pred WHERE pred_id = %1";
            firstUpdateT = "UPDATE sync_pred SET dor_kod = '%2', vd_id = '%3', name = '%4', sname = '%5', vname = '%6', gr_id = '%7',deleted = '%8', modified = '%9' WHERE pred_id = %1";
            firstUpdateTn = "UPDATE sync_pred SET dor_kod = '%2', vd_id = '%3', name = '%4', sname = '%5', vname = '%6', gr_id = '%7',deleted = %8, modified = '%9' WHERE pred_id = %1";
            firstInsertT = "INSERT INTO sync_pred (pred_id, dor_kod, vd_id, name, sname, vname, gr_id, deleted, added) VALUES (%1, '%2', '%3', '%4', '%5', '%6', '%7', '%8', '%9')";
            firstInsertTn = "INSERT INTO sync_pred (pred_id, dor_kod, vd_id, name, sname, vname, gr_id, deleted, added) VALUES (%1, '%2', '%3', '%4', '%5', '%6', '%7', %8, '%9')";
            dbTab = "PRED";
            formID = "21530904";
        }
        break;

        case PODR:
        {
            arg = 7;
            firstSelectT = "SELECT PO.Pred_id, PO.Podr_Id, PO.Podr_Nom, PO.Name, PO.Sname, PO.Vname, PO.Cor_Tip FROM PODR PO inner join PRED on PO.Pred_id = PRED.Pred_id  WHERE (PRED.Vd_Id between 900 and 999 and PRED.Gr_Id in (7,70)) or (PRED.Vd_Id = 0 and PRED.gr_id = 10)";
            secondSelectT = "SELECT * FROM sync_podr WHERE pred_id = %1 and podr_id = %2 and podr_num = %3 and name = '%4' and sname = '%5' and vname = '%6'";
            thirdSelectT = "SELECT * FROM sync_podr WHERE pred_id = %1 AND podr_id = %2";
            firstUpdateT = "UPDATE sync_podr SET podr_num = '%3', name = '%4', sname = '%5', vname = '%6', deleted = '%7', modified = '%8' WHERE pred_id = %1 AND podr_id = %2";
            firstUpdateTn = "UPDATE sync_podr SET podr_num = '%3', name = '%4', sname = '%5', vname = '%6', deleted = %7, modified = '%8' WHERE pred_id = %1 AND podr_id = %2";
            firstInsertT = "INSERT INTO sync_podr (pred_id, podr_id, podr_num, name, sname, vname, deleted, added) VALUES (%1, %2, '%3', '%4', '%5', '%6', '%7', '%8')";
            firstInsertTn = "INSERT INTO sync_podr (pred_id, podr_id, podr_num, name, sname, vname, deleted, added) VALUES (%1, %2, '%3', '%4', '%5', '%6', %7, '%8')";
            dbTab = "PODR";
            formID = "21530905";
        }
        break;

        case PERSONAL:
        {
            arg = 9;
            firstSelectT = "SELECT PO.Pred_id, PO.Sotr_Id, PO.Podr_id, PO.Dol_Id, PO.Name, PO.First_Name, PO.Last_Name, PO.PERSONID, PO.Cor_Tip FROM PERSONAL PO inner join PRED on PO.Pred_id = PRED.Pred_id  WHERE (PRED.Vd_Id between 900 and 999 and PRED.Gr_Id in (7,70)) or (PRED.Vd_Id = 0 and PRED.gr_id = 10)";
            secondSelectT = "SELECT * FROM sync_personal WHERE pred_id = %1 and sotr_id = %2 and podr_id = %3 and dol_id = '%4' and name = '%5' and first_name = '%6' and last_name = '%7' and personid = '%8'";
            thirdSelectT = "SELECT * FROM sync_personal WHERE pred_id = %1 AND sotr_id = %2";
            firstUpdateT = "UPDATE sync_personal SET podr_id = '%3', dol_id = '%4', name = '%5', first_name = '%6', last_name = '%7', personid = '%8', deleted = '%9', modified = '%10' WHERE pred_id = %1 AND sotr_id = %2";
            firstUpdateTn = "UPDATE sync_personal SET podr_id = '%3', dol_id = '%4', name = '%5', first_name = '%6', last_name = '%7', personid = '%8', deleted = %9, modified = '%10' WHERE pred_id = %1 AND sotr_id = %2";
            firstInsertT = "INSERT INTO sync_personal (pred_id, sotr_id, podr_id, dol_id, name, first_name, last_name, personid, deleted, added, login) VALUES (%1, %2, '%3', '%4', '%5', '%6', '%7', '%8', '%9', '%10', '%11')";
            firstInsertTn = "INSERT INTO sync_personal (pred_id, sotr_id, podr_id, dol_id, name, first_name, last_name, personid, deleted, added, login) VALUES (%1, %2, '%3', '%4', '%5', '%6', '%7', '%8', %9, '%10', '%11')";
            dbTab = "PERSONAL";
            formID = "21530902";
        }
        break;

        case MPS:
        {
            arg = 2;
            firstSelectT = "SELECT pred_v_id, pred_n_id FROM vertsv_mps";
            secondSelectT = "SELECT * FROM sync_vertsv_mps WHERE pred_v_id = %1 and pred_n_id = %2";
            thirdSelectT = "SELECT * FROM sync_vertsv_mps WHERE pred_v_id = %1 and pred_n_id = %2";
            firstUpdateT = "UPDATE sync_vertsv_mps SET WHERE pred_id = %1 AND sotr_id = %2";
            firstInsertT = "INSERT INTO sync_vertsv_mps (pred_v_id, pred_n_id) VALUES (%1, %2)";
            dbTab = "MPS";
            formID = "21530907";
        }
        break;
    }

    if(arg == 0)
        return "";

    auto temp = GetSid();
    auto table = getTable(strSrv, temp, formID);

    if(table.size() == 0)
        return QString("Не удалось начитать список %1. ").arg(dbTab);

    qDebug() << "Старт " << dbTab;
    QSqlQuery queryTo(m_dbSettings);
    foreach(const auto& tableValue, table)
    {
        countAll++;
        //qDebug() << countAll;
        QString secondSelect = secondSelectT;
        QString thirdSelect = thirdSelectT;
        QString firstUpdate = firstUpdateT;
        QString firstInsert = firstInsertT;
        std::vector<QString> args;
        for(unsigned i = 0; !m_bStopping.load() && i < arg; i++)
        {
            if(name != DOR && name != MPS && i == arg - 1)
            {
                args.push_back((tableValue[i] == "D")?QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss"):"NULL");
                if(args[i] != "NULL")
                    secondSelect += " and deleted is not NULL";
                else
                {
                    secondSelect += " and deleted is NULL";
                    firstUpdate = firstUpdateTn;
                    firstInsert = firstInsertTn;
                }
            }
            else
            {
                args.push_back(tableValue[i]);
                secondSelect = secondSelect.arg(args[i]);
            }
        }

        if (m_bStopping.load())
            break;

        if(!queryTo.exec(secondSelect))
            m_dbSettings.writeLog(QString("Не удалось выполнить запрос %1").arg(secondSelect), dBase::ASU);

        if (m_bStopping.load())
            break;

        if(!queryTo.next())
        {
            if(name == PODR || name == MPS || name == PERSONAL)
                thirdSelect = thirdSelect.arg(args[0], args[1]);
            else
                thirdSelect = thirdSelect.arg(args[0]);

            if(!queryTo.exec(thirdSelect))
                m_dbSettings.writeLog(QString("Не удалось выполнить запрос %1").arg(thirdSelect), dBase::ASU);

            if (queryTo.next())
            {
                for(unsigned i = 0; i < arg; i++)
                {
                    if(name != DOR && name != MPS && i == arg - 1)
                    {
                        firstUpdate = firstUpdate.arg(args[i]);
                        firstUpdate = firstUpdate.arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss"));
                    }
                    else
                        firstUpdate = firstUpdate.arg(args[i]);
                }

                if(!queryTo.exec(firstUpdate))
                    m_dbSettings.writeLog(QString("Не удалось выполнить запрос %1").arg(firstUpdate), dBase::ASU);
                else
                    countUp++;
            }
            else
            {
                for(unsigned i = 0; i < arg; i++)
                {
                    if(name != DOR && name != MPS && i == arg - 1)
                    {
                        firstInsert = firstInsert.arg(args[i]);
                        firstInsert = firstInsert.arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss"));
                        if(name == PERSONAL)
                        {
                            QString loginP = tran(args[4] + "_");
                            if(args[5] != "")
                                loginP += tran(QString(args[5][0]));
                            if(args[6] != "")
                                loginP += tran(QString(args[6][0]));

                            bool newLogin = false;
                            int newLoginNum = 0;
                            while(newLogin == false)
                            {
                               if(!queryTo.exec(QString("SELECT * FROM sync_personal WHERE login = '%1'").arg(loginP)))
                                   m_dbSettings.writeLog(QString("Не удалось выполнить запрос %1").arg(QString("SELECT * FROM sync_personal WHERE login = '%1'").arg(loginP)), dBase::ASU);
                               if (queryTo.next())
                               {
                                   newLoginNum++;
                                   if(newLoginNum > 1)
                                       loginP = loginP.mid(0, loginP.indexOf(QRegExp("[0-9]")) - 1);
                                   loginP = loginP + "_" + QString::number(newLoginNum);
                               }
                               else
                               {
                                   newLogin = true;
                               }
                            }
                            firstInsert = firstInsert.arg(loginP);
                        }
                    }
                    else
                        firstInsert = firstInsert.arg(args[i]);
                }

                if(!queryTo.exec(firstInsert))
                    m_dbSettings.writeLog(QString("Не удалось выполнить запрос %1").arg(firstInsert), dBase::ASU);
                else
                    countIn++;
            }
        }

        if (m_bStopping.load())
            break;
    }
    qDebug() << "Финиш " << dbTab;
    return QString("%1: добавлено %2, обновлено %3, всего %4. ").arg(dbTab, QString::number(countIn), QString::number(countUp), QString::number(countAll));
}

void AsuInstruct::process()
{
    QString strError;
    QDateTime tmWakeup;
    while(!m_bStopping.load())
    {
        EkasuiInstruct::asuDone = false;
        auto dateNow = QDateTime::currentDateTime();
        if  (m_dbSettings.Connect(strError))
        {
            m_dbSettings.writeLog("Старт синхронизации с АСУ-Ш-2", dBase::SDO);

            QSqlQuery queryTo(m_dbSettings.db);
            XmlSettings xmlSett;
            if (xmlSett.Load(m_dbSettings))
            {
                QString strSrv = xmlSett.GetValue("assh2/server");
                QString resultSTR = " Синхронизация АСУ-Ш-2 завершена. ";
                if (!m_bStopping.load())
                    resultSTR += updateDB(strSrv, DOR);
                if (!m_bStopping.load())
                    resultSTR += updateDB(strSrv, DOL);
                if (!m_bStopping.load())
                    resultSTR += updateDB(strSrv, PRED);
                if (!m_bStopping.load())
                    resultSTR += updateDB(strSrv, PODR);
                if (!m_bStopping.load())
                    resultSTR += updateDB(strSrv, PERSONAL);
                if (!m_bStopping.load())
                    resultSTR += updateDB(strSrv, MPS);
                m_dbSettings.writeLog(resultSTR, dBase::ASU);
            }            
            m_dbSettings.Close();

            dateNow = QDateTime::currentDateTime();
            tmWakeup = QDateTime(dateNow.date().addDays(1), QTime(0, 0));
        }
        else
        {
            dateNow = QDateTime::currentDateTime();
            tmWakeup = dateNow.addSecs(10 * 60);
        }

        EkasuiInstruct::asuDone = true;
        EkasuiInstruct::asuWait.wakeAll();

        while (!m_bStopping.load() && dateNow < tmWakeup)
        {
            QThread::sleep(static_cast<unsigned long>(std::min(10, int(dateNow.secsTo(tmWakeup)))));
            dateNow = QDateTime::currentDateTime();
        }
    }

    emit finished();
}

void AsuInstruct::stop()
{
    m_bStopping = true;
}
