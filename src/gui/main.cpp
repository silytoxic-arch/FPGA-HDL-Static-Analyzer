#include "gui/main_window.h"
#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::China));
    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale(QLocale::Chinese, QLocale::China),
                          QStringLiteral("qtbase"), QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        application.installTranslator(&qtTranslator);
    }
    QApplication::setApplicationName("FPGA HDL Static Analyzer");
    QApplication::setApplicationVersion("1.4.258");
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/branding/app-icon.png")));
    fpga::gui::MainWindow window;
    window.show();
    if (application.arguments().size() > 1) window.openProjectPath(application.arguments().at(1));
    return application.exec();
}
