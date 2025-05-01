#include "oauthserver.hpp"
#include <QUrlQuery>
#include <QHostAddress>
#include <QFile>
#include <QCoreApplication>
#include <QTextStream>

OAuthServer::OAuthServer(QObject* parent) : QTcpServer(parent) {}

void OAuthServer::start() {
    listen(QHostAddress::LocalHost, 8000);
}

void OAuthServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    if (socket->waitForReadyRead(3000)) {
        QString request = socket->readAll();
        QString firstLine = request.split("\r\n")[0];

        if (firstLine.startsWith("GET")) {
            QUrl url("http://localhost" + firstLine.split(" ")[1]);
            QString code = QUrlQuery(url).queryItemValue("code");

            if (!code.isEmpty()) {
                QString dir = QCoreApplication::applicationDirPath() + "/../login_frontend/";
                QFile htmlFile(dir + "success.html");
                QFile cssFile(dir + "style.css");
                QFile jsFile(dir + "confetti.js");

                QString htmlContent;

                if (htmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&htmlFile);
                    htmlContent = in.readAll();
                    htmlFile.close();

                    // Inline CSS
                    if (cssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QString css = QTextStream(&cssFile).readAll();
                        cssFile.close();
                        htmlContent.replace("<link rel=\"stylesheet\" href=\"style.css\">",
                                            "<style>" + css + "</style>");
                    }

                    // Inline JS
                    if (jsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QString js = QTextStream(&jsFile).readAll();
                        jsFile.close();
                        htmlContent.replace("<script src=\"confetti.js\"></script>",
                                            "<script>" + js + "</script>");
                    }
                } else {
                    htmlContent = "<html><body><h2>Login successful!</h2><p>(But we couldn't load the custom HTML file.)</p></body></html>";
                }

                QString response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n\r\n" +
                    htmlContent;

                socket->write(response.toUtf8());
                socket->flush();
                emit receivedCode(code);
            }
        }
    }

    socket->disconnectFromHost();
    socket->deleteLater();
}

