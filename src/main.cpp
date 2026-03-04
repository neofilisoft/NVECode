#include <QApplication>

#include "ui/main_window.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationDisplayName("NOC");
    app.setApplicationName("NOCompiler");
    app.setOrganizationName("Neofilisoft");

    MainWindow window;
    window.show();
    return app.exec();
}