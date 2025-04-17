#include "mainwindow.hpp"
#include "worker.hpp"
#include "auth.hpp"
#include "clone.hpp"
#include "analyze.hpp"
#include "globals.hpp"
#include "print_sorted.hpp"
#include "utils.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QThread>
#include <filesystem>
#include <git2.h>

namespace fs = std::filesystem;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    git_libgit2_init();

    QWidget* central = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(central);

    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);

    inputField = new QLineEdit(this);
    goButton = new QPushButton("Go", this);

    leftLayout->addWidget(inputField);
    leftLayout->addWidget(goButton);
    leftLayout->addStretch();

    resultArea = new QTextEdit(this);
    resultArea->setReadOnly(true);

    mainLayout->addWidget(leftPanel, 1);
    mainLayout->addWidget(resultArea, 2);
    setCentralWidget(central);
    resize(1000, 600);

    connect(goButton, &QPushButton::clicked, this, &MainWindow::startWorkerThread);
}

MainWindow::~MainWindow() {
    git_libgit2_shutdown();
}

void MainWindow::startWorkerThread() {
    resultArea->clear();  // Clear before new run starts

    QThread* thread = new QThread;
    Worker* worker = new Worker(inputField->text(), resultArea);

    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &Worker::run);
    connect(worker, &Worker::logMessage, this, &MainWindow::appendLog);
    connect(worker, &Worker::finalResult, this, &MainWindow::replaceWithFinalResult);
    connect(worker, &Worker::finished, thread, &QThread::quit);
    connect(worker, &Worker::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void MainWindow::appendLog(const QString& message) {
    resultArea->append(message);
}

void MainWindow::replaceWithFinalResult(const QString& summary) {
    resultArea->clear();
    resultArea->setPlainText(summary);
}
