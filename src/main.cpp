#include <string>
#include <iostream>

#include <QApplication>
#include "mainwindow.hpp"
#include "runTerminalMode.hpp"

int main(int argc, char* argv[]) {
    // Check for the "--nogui" flag
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--nogui") {
            return runTerminalMode();
        }
    }

    // Default to GUI mode
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
