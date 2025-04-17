#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private Q_SLOTS:
    void startWorkerThread();                // Launches the background thread
    void appendLog(const QString& message);  // Updates output live from thread
    void replaceWithFinalResult(const QString& summary);  // Replaces logs with final result

private:
    QLineEdit* inputField;
    QPushButton* goButton;
    QTextEdit* resultArea;
};

#endif // MAINWINDOW_HPP
