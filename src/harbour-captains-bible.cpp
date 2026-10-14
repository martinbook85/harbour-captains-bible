#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>
#include <QtQuick>
#include "biblemanager.h"

int main(int argc, char *argv[])
{
    // Registrera ModuleInfo för Qt MetaObject-systemet
    qRegisterMetaType<ModuleInfo>();

    // 1. Skapa Sailfish OS-applikationsobjektet
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setOrganizationName("org.myorg");
    app->setApplicationName("harbour-captains-bible");

    // 2. Skapa QML-vyn
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    // 3. Skapa vår SQLite/MyBible-motor
    BibleManager bibleManager;

    // 4. Exponera motorn till QML under namnet "bibleManager"
    view->rootContext()->setContextProperty("bibleManager", &bibleManager);

    // 5. Ladda huvud-QML-filen och visa fönstret
    view->setSource(SailfishApp::pathTo("qml/harbour-captains-bible.qml"));
    view->show();

    return app->exec();
}
