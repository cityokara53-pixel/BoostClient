#include <QTest>

#include "minecraft/auth/AuthSession.h"
#include "minecraft/auth/MinecraftAccount.h"

class AccountSessionTest final : public QObject {
    Q_OBJECT

   private slots:
    void offlineAccountUsesStandardSession()
    {
        const auto account = MinecraftAccount::createOffline("OfflineTester");
        auto session = std::make_shared<AuthSession>();
        session->launchMode = LaunchMode::Normal;

        account->fillSession(session);

        QCOMPARE(session->session, QString("-"));
        QCOMPARE(session->access_token, QString("0"));
        QCOMPARE(session->player_name, QString("OfflineTester"));
        QCOMPARE(session->uuid, MinecraftAccount::uuidFromUsername("OfflineTester").toString(QUuid::Id128));
        QCOMPARE(session->user_type, QString("offline"));
    }

    void offlineSessionMapsToLaunchArguments()
    {
        const auto account = MinecraftAccount::createOffline("OfflineTester");
        auto session = std::make_shared<AuthSession>();
        session->launchMode = LaunchMode::Normal;
        account->fillSession(session);

        const auto arguments = session->argumentMapping();

        QCOMPARE(arguments.value("auth_session"), QString("-"));
        QCOMPARE(arguments.value("auth_access_token"), QString("0"));
        QCOMPARE(arguments.value("auth_player_name"), QString("OfflineTester"));
        QCOMPARE(arguments.value("auth_uuid"), MinecraftAccount::uuidFromUsername("OfflineTester").toString(QUuid::Id128));
        QCOMPARE(arguments.value("user_type"), QString("offline"));
        QCOMPARE(arguments.value("user_properties"), QString("{}"));
    }

    void authenticatedAccountSessionRemainsUnchanged()
    {
        auto account = MinecraftAccount::createBlankMSA();
        auto* data = account->accountData();
        data->minecraftProfile.id = "0123456789abcdef0123456789abcdef";
        data->minecraftProfile.name = "LicensedPlayer";
        data->yggdrasilToken.token = "authenticated-access-token";

        auto session = std::make_shared<AuthSession>();
        session->launchMode = LaunchMode::Normal;
        account->fillSession(session);

        const auto arguments = session->argumentMapping();

        QCOMPARE(session->session, QString("token:authenticated-access-token:0123456789abcdef0123456789abcdef"));
        QCOMPARE(session->access_token, QString("authenticated-access-token"));
        QCOMPARE(session->player_name, QString("LicensedPlayer"));
        QCOMPARE(session->uuid, QString("0123456789abcdef0123456789abcdef"));
        QCOMPARE(session->user_type, QString("msa"));
        QCOMPARE(arguments.value("auth_session"), session->session);
        QCOMPARE(arguments.value("auth_access_token"), session->access_token);
        QCOMPARE(arguments.value("auth_player_name"), session->player_name);
        QCOMPARE(arguments.value("auth_uuid"), session->uuid);
        QCOMPARE(arguments.value("user_type"), session->user_type);
    }
};

QTEST_GUILESS_MAIN(AccountSessionTest)

#include "AccountSession_test.moc"
