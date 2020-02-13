#ifndef ASUINSTRUCT_H
#define ASUINSTRUCT_H

#include <QObject>

#include "dbsettings.h"

class AsuInstruct : public QObject
{
    Q_OBJECT

public:
    explicit AsuInstruct(const dBase &db, QObject *parent = nullptr);
    ~AsuInstruct();

    enum dbName{DOL, PERSONAL, DOR, MPS, PODR, PRED};

protected:
    dBase m_dbSettings;
    std::atomic_bool m_bStopping;

    QString updateDB(const QString &strSrv, dbName name);
    QString GetSid(QString login = "assh2", QString password = "assh2", int predId = 2678, int sotrId = 0);
    std::vector<std::vector<QString>> getTable(const QString &strSrv, QString sid, QString formID);
    QString tran(QString text);

signals:
    void finished();

public slots:
    void process();
    void stop();
};

#endif // ASUINSTRUCT_H
