#include "xmlsettings.h"
#include "dbsettings.h"

#include <qvariant.h>
#include <QDomDocument>
/*#include <QSqlDatabase>
#include <QSqlQuery>*/

XmlSettings::XmlSettings()
{
}

bool XmlSettings::Load(dBase &db)
{
    QString execString = "SELECT cast(convert_from(shablon, 'UTF8') as varchar) FROM docform WHERE docform_id = 10001";
    QSqlQuery qr(db);

    if(!qr.exec(execString) || !qr.first())
    {
        db.writeLog(QString("Не удалось выполнить запрос %1").arg(execString), dBase::MOD);
        return false;
    }

    QString strXml = qr.value(0).toString(), strError;
    QDomDocument doc;
    if (!doc.setContent(strXml, &strError))
    {
        db.writeLog(QString("Ошибка в настроечном XML: %1").arg(strError), dBase::MOD);
        return false;
    }

    QDomElement root = doc.documentElement();
    QDomNode node = root.firstChild();

    ParseNode(root, m_arNode);

    return true;
}

QString XmlSettings::GetValue(QString strFullPath) const
{
    return GetValue(strFullPath, m_arNode);
}

XmlSettings::Node XmlSettings::GetNode(QString strFullPath) const
{
    return GetNode(strFullPath, m_arNode);
}

bool XmlSettings::operator ==(const XmlSettings &xml) const
{
    return m_arNode == xml.m_arNode;
}

bool XmlSettings::operator !=(const XmlSettings &xml) const
{
    return  !this->operator==(xml);
}

QString XmlSettings::ParseNode(QDomNode &node, std::vector<Node> &arNode)
{
    QString strOut;
    arNode.clear();
    QDomNode child = node.firstChild();
    while (!child.isNull())
    {
        if (child.isText())
            strOut = child.nodeValue();
        else
        {
            Node nuNode;
            nuNode.m_strName = child.toElement().tagName();
            nuNode.m_strValue = ParseNode(child, nuNode.m_arNode);
            arNode.push_back(nuNode);
        }

        child = child.nextSibling();
    }

    return strOut;
}

QString XmlSettings::GetValue(QString strFullPath, const std::vector<Node> &arNode)
{
    int nPos = strFullPath.indexOf('/');
    QString str = nPos > 0 ? strFullPath.left(nPos) : strFullPath;
    strFullPath.remove(0, str.length() + 1);

    for (unsigned i = 0; i < arNode.size(); ++i)
    {
        if (str == arNode[i].m_strName)
            return strFullPath.length() > 0
                    ? GetValue(strFullPath, arNode[i].m_arNode)
                    : arNode[i].m_strValue;
    }

    return  "";
}

XmlSettings::Node XmlSettings::GetNode(QString strFullPath, const std::vector<Node> &arNode)
{
    int nPos = strFullPath.indexOf('/');
    QString str = nPos > 0 ? strFullPath.left(nPos) : strFullPath;
    strFullPath.remove(0, str.length() + 1);

    for (unsigned i = 0; i < arNode.size(); ++i)
    {
        if (str == arNode[i].m_strName)
            return strFullPath.length() > 0
                    ? GetNode(strFullPath, arNode[i].m_arNode)
                    : arNode[i];
    }

    return  Node();
}
