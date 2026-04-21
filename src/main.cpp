// entry point
#include <QApplication>
#include "mainwindow.h"
#include "thememanager.h"

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("XDDCC AutoClicker");
    app.setOrganizationName("XDDCC");

#ifdef _WIN32
    // boost timer resolution so intervals are tight
    timeBeginPeriod(1);
#endif

    ThemeManager themeManager;
    themeManager.loadBuiltInThemes();
    themeManager.applyTheme("Default");

    MainWindow w(&themeManager);
    w.show();

    int result = app.exec();

#ifdef _WIN32
    timeEndPeriod(1);
#endif
    return result;
}
