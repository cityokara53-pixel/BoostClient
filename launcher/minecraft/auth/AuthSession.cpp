#include "AuthSession.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

QString AuthSession::serializeUserProperties() const
{
    QJsonObject userAttrs;
    /*
    for (auto key : u.properties.keys())
    {
        auto array = QJsonArray::fromStringList(u.properties.values(key));
        userAttrs.insert(key, array);
    }
    */
    QJsonDocument value(userAttrs);
    return value.toJson(QJsonDocument::Compact);
}

QMap<QString, QString> AuthSession::argumentMapping() const
{
    return {
        { "auth_session", session },
        { "auth_access_token", access_token },
        { "auth_player_name", player_name },
        { "auth_uuid", uuid },
        { "user_properties", serializeUserProperties() },
        { "user_type", user_type },
    };
}

bool AuthSession::MakeOffline(QString offline_playername)
{
    session = "-";
    access_token = "0";
    player_name = offline_playername;
    return true;
}

void AuthSession::MakeDemo(QString name, QString u)
{
    uuid = u;
    session = "-";
    access_token = "0";
    player_name = name;
    launchMode = LaunchMode::Demo;
};
