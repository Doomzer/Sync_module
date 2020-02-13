#ifndef CONNECTORSDO_H
#define CONNECTORSDO_H

#include <QObject>
#include <QDateTime>
#include <QReadWriteLock>

class ConnectorSdo
{
    static QString m_strUrl;
    static QString m_appId;
    static QString m_secKey;

    static QReadWriteLock m_lockSettings;
    static QString m_strServer;
    static std::atomic_int m_nMeasureGroup;

public:
    struct memberResult
    {
        int m_shSdoStatusId;
        int m_dfSdoPoints;
        int m_prSdoEvaluation;
        int m_nSdoDuration;
        QDateTime m_tmSdoTimeEnd;
        QDateTime m_tmSdoTimeStart;
        int m_MemberId;
    };

    //static QString sdo;

public:
    ConnectorSdo();

    static void SetServer(QString strServer, int nMeasureGroup);
    static QString GetServer();
    static int GetMeasureGroup()        { return m_nMeasureGroup; }

    static std::vector<QString> GetPerson(QString tpPerson, QString &strError);
    static int CreateMeasure(int contentId, QString strName, QDateTime tmBegin, QDateTime tmEnd);
    static int MemberToMeasure(int meId, int persId);

    static bool GetMemberResult(int mmId, memberResult &result);

protected:
    static void Add2Group(int meId);
};

#endif // CONNECTORSDO_H
