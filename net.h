#ifndef NET_H
#define NET_H

#include <QObject>

#include <QtCore>
#include <QNetworkAccessManager>
#include <QNetworkReply>

enum netType{GET = 1, POST};

class Net : public QObject
{
Q_OBJECT
  QNetworkAccessManager *manager;
  QNetworkRequest request;
private slots:
  void replyFinished(QNetworkReply *);
public:
  QByteArray CheckSite(QString url, netType type);
  QByteArray CheckSite(QString url, QString formID, netType type);
};

#endif // NET_H
