#include "mainwindow.hpp"
#include "worker.hpp"
#include "auth.hpp"
#include "clone.hpp"
#include "analyze.hpp"
#include "globals.hpp"
#include "print_sorted.hpp"
#include "utils.hpp"
#include "oauthserver.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QThread>
#include <QDesktopServices>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
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
    loginButton = new QPushButton("Login with GitHub", this);
    loginStatusLabel = new QLabel(this);
    loginStatusLabel->setStyleSheet("color: green;");
    loginStatusLabel->setVisible(false);

    leftLayout->addWidget(inputField);
    leftLayout->addWidget(goButton);
    leftLayout->addWidget(loginButton);
    leftLayout->addWidget(loginStatusLabel);
    leftLayout->addStretch();

    resultArea = new QTextEdit(this);
    resultArea->setReadOnly(true);

    mainLayout->addWidget(leftPanel, 1);
    mainLayout->addWidget(resultArea, 2);
    setCentralWidget(central);
    resize(1000, 600);

    connect(goButton, &QPushButton::clicked, this, [this]() {
        startWorkerThread(inputField->text());
    });

    connect(inputField, &QLineEdit::returnPressed, this, [this]() {
        startWorkerThread(inputField->text());
    });

    connect(loginButton, &QPushButton::clicked, this, &MainWindow::startGitHubLogin);
}

MainWindow::~MainWindow() {
    git_libgit2_shutdown();
}

void MainWindow::startWorkerThread(const QString& token) {
    resultArea->clear();

    QThread* thread = new QThread;
    Worker* worker = new Worker(token, resultArea);
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

void MainWindow::startGitHubLogin() {
    const QString clientId = "Ov23liVQ3EBVLbcP681k";
    QUrl loginUrl("https://github.com/login/oauth/authorize");

    QUrlQuery query;
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("scope", "repo");
    query.addQueryItem("redirect_uri", "http://localhost:8000/callback");
    loginUrl.setQuery(query);

    QDesktopServices::openUrl(loginUrl);

    OAuthServer* server = new OAuthServer(this);
    connect(server, &OAuthServer::receivedCode, this, &MainWindow::handleOAuthCode);
    server->start();
}

void MainWindow::handleOAuthCode(const QString& code) {
    const QString clientId = "Ov23liVQ3EBVLbcP681k";
    QString clientSecret = getClientSecret();

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);

    QUrl url("https://github.com/login/oauth/access_token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Accept", "application/json");

    QUrlQuery params;
    params.addQueryItem("client_id", clientId);
    params.addQueryItem("client_secret", clientSecret);
    params.addQueryItem("code", code);
    params.addQueryItem("redirect_uri", "http://localhost:8000/callback");

    QNetworkReply* reply = manager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished, [this, reply, manager]() {
        QByteArray response = reply->readAll();
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(response);
        QJsonObject obj = doc.object();
        QString accessToken = obj["access_token"].toString();

        if (!accessToken.isEmpty()) {
            QNetworkRequest userRequest(QUrl("https://api.github.com/user"));
            userRequest.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
            userRequest.setRawHeader("User-Agent", "GitHubLangStatsApp");

            QNetworkReply* userReply = manager->get(userRequest);

            connect(userReply, &QNetworkReply::finished, [this, userReply, accessToken]() {
                QByteArray userResponse = userReply->readAll();
                userReply->deleteLater();

                QJsonDocument userDoc = QJsonDocument::fromJson(userResponse);
                QJsonObject userObj = userDoc.object();
                QString username = userObj["login"].toString();

                if (!username.isEmpty()) {
                    loginStatusLabel->setText("Logged in as " + username);
                    loginStatusLabel->setVisible(true);
                    startWorkerThread(accessToken);  // ✅ Trigger analysis
                } else {
                    QMessageBox::warning(this, "Error", "Failed to fetch GitHub username.");
                }
            });
        } else {
            QMessageBox::warning(this, "Error", "Failed to retrieve GitHub token.");
        }
    });
}
