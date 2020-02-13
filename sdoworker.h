#ifndef SDOWORKER_H
#define SDOWORKER_H

#include <QObject>

#include"dbsettings.h"

class XmlSettings;
class EkasuiInstruct;

class SdoWorker : public QObject
{
    Q_OBJECT

    dBase m_database;
    std::atomic_bool m_bStopping;
    const EkasuiInstruct &m_instruct;

public:
    explicit SdoWorker(const EkasuiInstruct &instruct, const dBase &database, QObject *parent = nullptr);
    ~SdoWorker();

    void SetSettings(const XmlSettings &xml);

protected:
    void CheckInstruct(QString strEkasuiServer);

signals:
    void finished();

public slots:
    void process();
    void stop();
};

#endif // SDOWORKER_H
