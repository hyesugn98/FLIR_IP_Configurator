#include "header/mainwindow.h"
#include "header/mainwindow.h"
#include "framelesswindow/framelesswindow.h"
#include "header/DarkStyle.h"
#include <QApplication>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle(new DarkStyle);
    QApplication::setPalette(QApplication::style()->standardPalette());
    FramelessWindow framelessWindow;
    framelessWindow.setWindowTitle("FLIR IP Configurator");
    framelessWindow.setWindowIcon(QIcon(":/image/logo/FLIR_white.png"));
    MainWindow *mainWindow = new MainWindow;

    framelessWindow.setContent(mainWindow);
    mainWindow->setWindowTitle("FLIR IP Configurator");
    framelessWindow.show();

    return a.exec();
}
