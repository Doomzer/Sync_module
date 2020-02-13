#include "connectorsdo.h"
#include "net.h"

#include <QReadLocker>
#include <QWriteLocker>

//QString ConnectorSdo::sdo;

QString ConnectorSdo::m_strUrl("/service/v2/persons");

// Боевой сервер
//QString ConnectorSdo::m_appId("new.sdo.rzd");
//QString ConnectorSdo::m_secKey("l#9wxiW!");
//QString ConnectorSdo::m_strServer("sdo.rzd/lms");
std::atomic_int ConnectorSdo::m_nMeasureGroup(6446);

// Тестовый сервер
QString ConnectorSdo::m_appId("AOSSH");
QString ConnectorSdo::m_secKey("ASDhjkuh456$");
QString ConnectorSdo::m_strServer("portal.sdo.rzd.ru/lmscluster");
//std::atomic_int ConnectorSdo::m_nMeasureGroup(6446);

QReadWriteLock ConnectorSdo::m_lockSettings;

ConnectorSdo::ConnectorSdo()
{
}

void ConnectorSdo::SetServer(QString strServer, int nMeasureGroup)
{
    QWriteLocker locker(&m_lockSettings);
    m_strServer = strServer;
    m_nMeasureGroup = nMeasureGroup;
}

QString ConnectorSdo::GetServer()
{
    QReadLocker locker(&m_lockSettings);
    return m_strServer;
}

std::vector<QString> ConnectorSdo::GetPerson(QString tpPerson, QString &strError)
{
    std::vector<QString> replay;

    Net handler;
    QString requestString = "https://%1%2?pwtabnumber=%3&appid=%4&secretkey=%5";

    QString strServer = GetServer();

    requestString = requestString.arg(strServer, m_strUrl, tpPerson, m_appId, m_secKey);
    QByteArray ba = requestString.toUtf8();

    QString requestStringReady = "https://%1%2?pwtabnumber=%3&appid=%4&sign=%5";
    requestStringReady = requestStringReady.arg(strServer, m_strUrl, tpPerson, m_appId, QString(QCryptographicHash::hash(ba, QCryptographicHash::Md5).toHex().toUpper()));

    auto temp = handler.CheckSite(requestStringReady, GET);
    QJsonDocument document = QJsonDocument::fromJson(temp);
    if (document.isArray() && document.array().size() >0)
    {
        QJsonArray tempA = document.array();
        QJsonValue tempB = tempA[0];
        QJsonObject tempC = tempB.toObject();
        replay.push_back(tempC.value("personemail").toString());
        replay.push_back(tempC.value("personid").toString());
    }
    else
        strError = QString(requestStringReady);

    return replay;
}

int ConnectorSdo::CreateMeasure(int contentId, QString strName, QDateTime tmBegin, QDateTime tmEnd)
{
    int meId(0);

    Net handler;
    QString requestString = "https://%1%2?contentid=%3&mecontenttype=0&meeduform=0&meenddate=%4&mename=%5&mestartdate=%6&metype=1&appid=%7&secretkey=%8";

    auto tempEnd = QDateTime::currentDateTime();
    tempEnd.setTime(QTime(18,0,0,0));

    QString meenddate = tempEnd.toString("yyyy-MM-dd HH:mm:ss.000");
    QString mestartdate = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:MM:ss.000");

    requestString = requestString.arg(GetServer(), m_strUrl, QString::number(contentId), tmEnd.toString("yyyy-MM-dd HH:mm:ss.000"), strName, tmBegin.toString("yyyy-MM-dd HH:MM:ss.000"),
                                      m_appId, m_secKey);

    QByteArray ba = requestString.toUtf8();
    QString sign = QString(QCryptographicHash::hash(ba, QCryptographicHash::Md5).toHex().toUpper());
    requestString = QUrl::toPercentEncoding(requestString,"/:?=&");
    requestString = requestString.left(requestString.indexOf("secretkey"));
    requestString += "sign=" + sign;
    auto temp = handler.CheckSite(requestString, POST);
    QJsonDocument document = QJsonDocument::fromJson(temp);
    if (document.isObject())
    {
        QJsonObject tempObj = document.object();
        meId = tempObj.value("meid").toInt();
    }

    if (0 != meId)
    {
        Add2Group(meId);
    }

    return meId;
}

void ConnectorSdo::Add2Group(int meId)
{
    Net handler;
    QString requestString = "https://%1%2/measureGroups/%3/childMeasures/%4?appid=%5&secretkey=%6";

    requestString = requestString.arg(GetServer(), m_strUrl, QString::number(m_nMeasureGroup),
                                      QString::number(meId), m_appId, m_secKey);

    QByteArray ba = requestString.toUtf8();
    QString sign = QString(QCryptographicHash::hash(ba, QCryptographicHash::Md5).toHex().toUpper());
    requestString = QUrl::toPercentEncoding(requestString,"/:?=&");
    requestString = requestString.left(requestString.indexOf("secretkey"));
    requestString += "sign=" + sign;
    auto temp = handler.CheckSite(requestString, POST);
    QString replay = QTextCodec::codecForMib(106)->toUnicode(temp);
}

int ConnectorSdo::MemberToMeasure(int meId, int persId)
{
    int mmId(0);

    Net handler;
    QString requestString = "https://%1%2/measures/%3/members/%4?regtype=1&appid=%5&secretkey=%6";

    requestString = requestString.arg(GetServer(), m_strUrl, QString::number(meId), QString::number(persId), m_appId, m_secKey);

    QByteArray ba = requestString.toUtf8();
    QString sign = QString(QCryptographicHash::hash(ba, QCryptographicHash::Md5).toHex().toUpper());
    requestString = QUrl::toPercentEncoding(requestString,"/:?=&");
    requestString = requestString.left(requestString.indexOf("secretkey"));
    requestString += "sign=" + sign;
    auto temp = handler.CheckSite(requestString, POST);
    QString replay = QTextCodec::codecForMib(106)->toUnicode(temp);
    mmId = replay.toInt();

    return mmId;
}

bool ConnectorSdo::GetMemberResult(int mmId, ConnectorSdo::memberResult &result)
{
    Net handler;
    QString requestString = "https://%1%2/measureResults/%3?appid=%4&secretkey=%5";

    requestString = requestString.arg(GetServer(), m_strUrl, QString::number(mmId), m_appId, m_secKey);

    QByteArray ba = requestString.toUtf8();
    QString sign = QString(QCryptographicHash::hash(ba, QCryptographicHash::Md5).toHex().toUpper());
    requestString = QUrl::toPercentEncoding(requestString,"/:?=&");
    requestString = requestString.left(requestString.indexOf("secretkey"));
    requestString += "sign=" + sign;
    auto temp = handler.CheckSite(requestString, GET);
    QJsonDocument document = QJsonDocument::fromJson(temp);
    if (document.isObject())
    {
        QJsonObject tempObj = document.object();
        result.m_shSdoStatusId = tempObj.value("mrstatusid").toVariant().toInt();
        result.m_dfSdoPoints = tempObj.value("mrscore").toVariant().toInt();
        result.m_prSdoEvaluation = tempObj.value("mrscoreinpercent").toVariant().toInt();
        result.m_nSdoDuration = tempObj.value("mrduration").toVariant().toInt();
        result.m_tmSdoTimeEnd = tempObj.value("mrendtime").toVariant().toDateTime();
        result.m_tmSdoTimeStart = tempObj.value("mrstarttime").toVariant().toDateTime();
        result.m_MemberId = tempObj.value("mmid").toVariant().toInt();
        return true;
    }

    return false;
}
