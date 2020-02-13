#include "net.h"

void Net::replyFinished(QNetworkReply *reply)
{
    if (reply->error())
    {
        //qDebug() << reply->errorString();
        return;
    }
}

QByteArray Net::CheckSite(QString url, netType type)
{
  QByteArray answer;
  manager = new QNetworkAccessManager();
  connect(manager, &QNetworkAccessManager::finished, this, &Net::replyFinished);
  request.setUrl(QUrl(url));
  QNetworkReply *reply;
  if (type == GET)
      reply = manager->get(request);
  else
  {
    QByteArray temp;
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/x-www-form-urlencoded"));
    reply = manager->post(request, temp);
  }
  QEventLoop loop;
  connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
  loop.exec();
  if (reply->error())
  {
      qDebug() << "Ошибка: " + reply->errorString();
  }
  else
  {
      answer = reply->readAll();
      //qDebug() << answer;
      //answer = QJsonDocument::fromJson(temp);
  }

  return answer;
}

QByteArray Net::CheckSite(QString url, QString formID, netType type)
{
  QByteArray answer;
  manager = new QNetworkAccessManager();
  connect(manager, &QNetworkAccessManager::finished, this, &Net::replyFinished);
  request.setUrl(QUrl(url));
  QNetworkReply *reply;
  if (type == GET)
      reply = manager->get(request);
  else
  {
    QByteArray temp;
    temp.append(QString("{predId:0,sotrId:0,formId:%1,periodId:0,\"filterStr\":\"\"}").arg(formID));
    QByteArray postDataSize = QByteArray::number(temp.size());
    request.setRawHeader("User-Agent", "My app name v0.1");
    request.setRawHeader("X-Custom-User-Agent", "My app name v0.1");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);
    reply = manager->post(request, temp);
  }
  QEventLoop loop;
  connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
  loop.exec();
  if (reply->error())
  {
      qDebug() << "Ошибка: " + reply->errorString();
  }
  else
  {
      answer = reply->readAll();
      //qDebug() << answer;
      //answer = QJsonDocument::fromJson(temp);
  }

  return answer;
}
