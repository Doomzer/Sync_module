#ifndef INSTRUCTWORKER_H
#define INSTRUCTWORKER_H

//#include <QObject>
#include <QDate>

#include "dbsettings.h"

class _ns1__GetJpiResponse_JPI;
class EkasuiInstruct;

class InstructWorker : public QObject
{
    Q_OBJECT

public:
    enum EkasuiStatus {
        EkasuiStatus_None = 1, // Нет данных из АОС
        EkasuiStatus_InstructNone, // По работе из ЕКАСУ-И нет инструктажа
        EkasuiStatus_InstructAssigned, // Инструктаж назначен
        EkasuiStatus_InstructExecuted // Инструктаж выполнен (с тем или иным результатом)
    };

    enum SdoStatus {
        SdoStatus_NotPassed = 0,
        SdoStatus_Passed = 1,
        SdoStatus_InProcess = 3,
        SdoStatus_NotStarted = 4
    };


private:
    QList<unsigned> m_listDor;
    int m_timeZone;
    //QDate m_currentDay;
    dBase m_database;

    std::atomic_bool m_bStopping;

    // std::map<PersonalId, SdoPersonId>
    std::map<int, int> m_mapPerson2Sdo;

    const EkasuiInstruct &m_instruct;

public:
    explicit InstructWorker(const EkasuiInstruct &instruct, const dBase &db, QList<unsigned> &listDor, int timeZone, QObject *parent = nullptr);

signals:
    void finished();

public slots:
    void process();
    void stop();

public:
    static bool SendEkasuiResult(dBase &database, QString strServer, int jpInstrId, int statusEkasui, int statusSdo);

protected:
    void ProcessEkasui(QDate dtCurDay);

    void ProcessJPI(bool bSendResult, QString strServer, QDate dtCurDay, int predId, int podrId, _ns1__GetJpiResponse_JPI &resp);

    // std::pair<JpId, SdoContentId>
    std::pair<int, int> GetJP(_ns1__GetJpiResponse_JPI &resp, QString &strName, bool bCreate);

    // std::pair<PersonalId, SdoPersonId>
    std::pair<int, int> GetPersonalId(int predId, int podrId, _ns1__GetJpiResponse_JPI &resp, bool bCreate);

    int GetMeasure(int jpId, QDate dtCurDay, int contentId, QString strName, bool bCreate);

    // возвращаем true, если выполнены все назначенные инструктажи
    // в противном случае возвращается false
    //bool CheckInstruct(QString strEkasuiServer);
};

#endif // INSTRUCTWORKER_H
