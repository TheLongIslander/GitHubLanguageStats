#ifndef OAUTHSERVER_HPP
#define OAUTHSERVER_HPP

#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <QString>

class OAuthServer : public QTcpServer {
    Q_OBJECT
public:
    explicit OAuthServer(QObject* parent = nullptr);
    void start();

signals:
    void receivedCode(const QString& code);

protected:
    void incomingConnection(qintptr socketDescriptor) override;
};

#endif // OAUTHSERVER_HPP
