#ifndef XMLSETTINGS_H
#define XMLSETTINGS_H

#include <QObject>

class dBase;
class QDomNode;

class XmlSettings
{
public:
    struct Node
    {
        QString m_strName;
        QString m_strValue;
        std::vector<Node> m_arNode;

        QString GetValue(QString strFullPath) const
        {
            return XmlSettings::GetValue(strFullPath, m_arNode);
        }

        Node GetNode(QString strFullPath) const
        {
            return XmlSettings::GetNode(strFullPath, m_arNode);
        }
        bool operator == (const Node &node) const
        {
            return      m_strName == node.m_strName
                    &&  m_strValue == node.m_strValue
                    &&  m_arNode == node.m_arNode;
        }
        bool operator != (const Node &node) const
        {
            return  !this->operator==(node);
        }

    };

protected:
    std::vector<Node> m_arNode;

public:
    XmlSettings();

    bool Load(dBase &db);

    QString GetValue(QString strFullPath) const;
    Node GetNode(QString strFullPath) const;

    bool operator == (const XmlSettings &xml) const;
    bool operator != (const XmlSettings &xml) const;

protected:
    QString ParseNode(QDomNode &node, std::vector<Node> &arNode);

    static QString GetValue(QString strFullPath, const std::vector<Node> &arNode);
    static Node GetNode(QString strFullPath, const std::vector<Node> &arNode);
};

#endif // XMLSETTINGS_H
