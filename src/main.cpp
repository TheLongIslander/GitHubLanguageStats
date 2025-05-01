#include <string>
#include <iostream>
#include "runTerminalMode.hpp"

#ifdef QT_GUI_LIB
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include "mainwindow.hpp"
#endif

bool guiSupported() {
#if defined(Q_OS_LINUX)
    return getenv("DISPLAY") != nullptr || getenv("WAYLAND_DISPLAY") != nullptr;
#elif defined(Q_OS_MAC)
    return true;  // GUI is always available if you're not SSH'd into macOS
#elif defined(Q_OS_WIN)
    return true;  // Windows always has GUI unless explicitly built without
#else
    return false;
#endif
}

int main(int argc, char* argv[]) {
    // If user passed --nogui, run terminal mode
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--nogui") {
            return runTerminalMode();
        }
    }

#ifdef QT_GUI_LIB
    if (!guiSupported()) {
        std::cerr << "No GUI environment detected. Launching in terminal mode (--nogui)...\n";
        return runTerminalMode();
    }

    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
#else
    std::cerr << "Qt GUI support not available. Running in terminal mode...\n";
    return runTerminalMode();
#endif
}
