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
            QUrlQuery query(url);
            QString code = query.queryItemValue("code");
            QString error = query.queryItemValue("error");

            QString htmlFile = (code.isEmpty() || !error.isEmpty())
                               ? "error.html"
                               : "success.html";

            QString dir = QCoreApplication::applicationDirPath() + "/../login_frontend/";
            QFile htmlFileObj(dir + htmlFile);
            QFile cssFile(dir + "style.css");
            QFile jsFile(dir + "confetti.js");

            QString htmlContent;

            if (htmlFileObj.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&htmlFileObj);
                htmlContent = in.readAll();
                htmlFileObj.close();

                if (cssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString css = QTextStream(&cssFile).readAll();
                    cssFile.close();
                    htmlContent.replace("<link rel=\"stylesheet\" href=\"style.css\">",
                                        "<style>" + css + "</style>");
                }

                if (jsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString js = QTextStream(&jsFile).readAll();
                    jsFile.close();
                    htmlContent.replace("<script src=\"confetti.js\"></script>",
                                        "<script>" + js + "</script>");
                }
            } else {
                htmlContent = "<html><body><h2>Something went wrong.</h2></body></html>";
            }

            QString response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n\r\n" +
                htmlContent;

            socket->write(response.toUtf8());
            socket->flush();

            if (!code.isEmpty())
                emit receivedCode(code);
        }
    }

    socket->disconnectFromHost();
    socket->deleteLater();
}
