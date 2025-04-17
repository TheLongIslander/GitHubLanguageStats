#ifndef WORKER_HPP
#define WORKER_HPP

#include <QObject>
#include <QTextEdit>
#include <QString>
#include <nlohmann/json.hpp>

class Worker : public QObject {
    Q_OBJECT

public:
    Worker(QString input, QTextEdit* outputArea);

signals:
    void finished();
    void logMessage(const QString& message);
    void finalResult(const QString& summary);
public slots:
    void run();

private:
    QString input;
    QTextEdit* outputArea;
};

#endif // WORKER_HPP
