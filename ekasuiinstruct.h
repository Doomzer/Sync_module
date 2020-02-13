#ifndef EKASUIINSTRUCT_H
#define EKASUIINSTRUCT_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QReadWriteLock>

#include "dbsettings.h"
#include "xmlsettings.h"

class InstructWorker;

class EkasuiInstruct : public QObject
{
    Q_OBJECT

protected:
    std::atomic_uint m_nThreadCount;
    std::atomic_bool m_bNeedRestart;
    std::atomic_bool m_bWaitingAsu;

    mutable QReadWriteLock m_lockSettings;
    XmlSettings m_xmlSettings;

    dBase m_database;

public:
    static QWaitCondition asuWait;
    static QMutex mutex;
    static bool asuDone;

public:
    explicit EkasuiInstruct(const dBase &db, QObject *parent = nullptr);
    ~EkasuiInstruct();

    void SetSettings(const XmlSettings &xmlSett);
    bool IsEnabled() const;

protected:
    void StartProcessed();
    static void StartProcessed(EkasuiInstruct *pObject);
    //void StartMzd();

    void StopWorking();

signals:
    void finished();
    void stopAll();

public slots:
    void dbNotify(const QString &strNotify);
    void onWorkerFinished();
};

#endif // EKASUIINSTRUCT_H
