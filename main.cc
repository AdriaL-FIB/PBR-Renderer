// Author: Marc Comino 2020

#include <QApplication>
#include <QSurfaceFormat>
#include "./main_window.h"

#ifdef WIN32
#include "Windows.h"
#include "Wincon.h"
#endif // WIN32


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QSurfaceFormat f;
    f.setVersion(3,3);
    f.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(f);

#ifdef WIN32
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif // WIN32

    gui::MainWindow w;
    w.show();

    return a.exec();
}
