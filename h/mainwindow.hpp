#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private Q_SLOTS:
    void startWorkerThread(const QString& token);    // Launch with token (manual or OAuth)
    void appendLog(const QString& message);          // Updates output live from thread
    void replaceWithFinalResult(const QString& summary); // Replaces logs with final result
    void startGitHubLogin();                         // Opens GitHub login page
    void handleOAuthCode(const QString& code);       // Receives and handles GitHub code

private:
    QLineEdit* inputField;
    QPushButton* goButton;
    QPushButton* loginButton;
    QTextEdit* resultArea;
    QLabel* loginStatusLabel;
};

#endif // MAINWINDOW_HPP
