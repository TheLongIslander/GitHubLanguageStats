#include "oauthserver.hpp"
#include <QUrlQuery>
#include <QHostAddress>

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
                QString response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n\r\n"
                    "<html><body><h2>GitHub login successful!</h2>You may now close this window.</body></html>";

                socket->write(response.toUtf8());
                socket->flush();
                emit receivedCode(code);
            }
        }
    }

    socket->disconnectFromHost();
    socket->deleteLater();
}
