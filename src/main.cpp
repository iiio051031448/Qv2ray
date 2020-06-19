#include "Qv2rayApplication.hpp"
#include "common/QvHelpers.hpp"
#include "core/handler/ConfigHandler.hpp"

#include <QFileInfo>
#include <QLocale>
#include <QProcess>
#include <QSslSocket>
#include <csignal>
#include <memory>

void signalHandler(int signum)
{
    std::cout << "Qv2ray: Interrupt signal (" << signum << ") received." << std::endl;
    ExitQv2ray();
    qvApp->exit(-99);
}

int RunQv2rayApplicationScoped(int argc, char *argv[])
{
    Qv2rayApplication app(argc, argv);

    if (!app.SetupQv2ray())
        return 0;

    LOG("LICENCE", NEWLINE                                                                                               //
        "This program comes with ABSOLUTELY NO WARRANTY." NEWLINE                                                        //
        "This is free software, and you are welcome to redistribute it" NEWLINE                                          //
        "under certain conditions." NEWLINE                                                                              //
            NEWLINE                                                                                                      //
        "Copyright (c) 2019-2020 Qv2ray Development Group." NEWLINE                                                      //
            NEWLINE                                                                                                      //
        "Libraries that have been used in Qv2ray are listed below (Sorted by date added):" NEWLINE                       //
        "Copyright (c) 2020 xyz347 (@xyz347): X2Struct (Apache)" NEWLINE                                                 //
        "Copyright (c) 2011 SCHUTZ Sacha (@dridk): QJsonModel (MIT)" NEWLINE                                             //
        "Copyright (c) 2016 Singein (@Singein): ScreenShot (MIT)" NEWLINE                                                //
        "Copyright (c) 2020 Itay Grudev (@itay-grudev): SingleApplication (MIT)" NEWLINE                                 //
        "Copyright (c) 2020 paceholder (@paceholder): nodeeditor (Qv2ray group modified version) (BSD-3-Clause)" NEWLINE //
        "Copyright (c) 2019 TheWanderingCoel (@TheWanderingCoel): ShadowClash (launchatlogin) (GPLv3)" NEWLINE           //
        "Copyright (c) 2020 Ram Pani (@DuckSoft): QvRPCBridge (WTFPL)" NEWLINE                                           //
        "Copyright (c) 2019 ShadowSocks (@shadowsocks): libQtShadowsocks (LGPLv3)" NEWLINE                               //
        "Copyright (c) 2015-2020 qBittorrent (Anton Lashkov) (@qBittorrent): speedplotview (GPLv2)" NEWLINE              //
        "Copyright (c) 2020 Diffusions Nu-book Inc. (@nu-book): zxing-cpp (Apache)" NEWLINE                              //
        "Copyright (c) 2020 feiyangqingyun: QWidgetDemo (Mulan PSL v1)" NEWLINE                                          //
            NEWLINE)                                                                                                     //
    //
#ifdef QT_DEBUG
    std::cerr << "WARNING: =================== This is a debug build, many features are not stable enough. ===================" << std::endl;
#endif
    //
    // Qv2ray Initialize, find possible config paths and verify them.
    if (!app.FindAndCreateInitialConfiguration())
    {
        LOG(MODULE_INIT, "Cannot find or create initial configuration file.")
        return -1;
    }
    if (!app.LoadConfiguration())
    {
        LOG(MODULE_INIT, "Cannot load existing configuration file.")
        return -2;
    }

    // Check OpenSSL version for auto-update and subscriptions
    auto osslReqVersion = QSslSocket::sslLibraryBuildVersionString();
    auto osslCurVersion = QSslSocket::sslLibraryVersionString();
    LOG(MODULE_NETWORK, "Current OpenSSL version: " + osslCurVersion)

    if (!QSslSocket::supportsSsl())
    {
        LOG(MODULE_NETWORK, "Required OpenSSL version: " + osslReqVersion)
        LOG(MODULE_NETWORK, "OpenSSL library MISSING, Quitting.")
        QvMessageBoxWarn(nullptr, QObject::tr("Dependency Missing"),
                         QObject::tr("Cannot find openssl libs") + NEWLINE +
                             QObject::tr("This could be caused by a missing of `openssl` package in your system.") + NEWLINE +
                             QObject::tr("If you are using an AppImage from Github Action, please report a bug.") + NEWLINE + //
                             NEWLINE + QObject::tr("Technical Details") + NEWLINE +                                           //
                             "OSsl.Rq.V=" + osslReqVersion + NEWLINE +                                                        //
                             "OSsl.Cr.V=" + osslCurVersion);
        return -3;
    }

    app.InitilizeGlobalVariables();

#ifndef Q_OS_WIN
    signal(SIGUSR1, [](int) { ConnectionManager->RestartConnection(); });
    signal(SIGUSR2, [](int) { ConnectionManager->StopConnection(); });
#endif
    return app.RunQv2ray();
}

int main(int argc, char *argv[])
{
#ifndef Q_OS_WIN
    // Register signal handlers.
    signal(SIGINT, signalHandler);
    signal(SIGHUP, signalHandler);
    signal(SIGKILL, signalHandler);
    signal(SIGTERM, signalHandler);
#endif
    //
    // This line must be called before any other ones, since we are using these
    // values to identify instances.
    QApplication::setApplicationVersion(QV2RAY_VERSION_STRING);
    //
#ifdef QT_DEBUG
    QApplication::setApplicationName("qv2ray_debug");
    QApplication::setApplicationDisplayName("Qv2ray - " + QObject::tr("Debug version"));
#else
    QApplication::setApplicationName("qv2ray");
    QApplication::setApplicationDisplayName("Qv2ray");
#endif
    //
    // parse the command line before starting as a Qt application
    if (!Qv2rayApplication::PreInitilize(argc, argv))
        return -1;
    const auto rcode = RunQv2rayApplicationScoped(argc, argv);
    switch (rcode)
    {
        case QV2RAY_EXITCODE_NEWVERSION:
        {
            QProcess::startDetached(Qv2rayProcessArgument._qvNewVersionPath, {});
            break;
        }
        default: break;
    }
    return rcode;
}
