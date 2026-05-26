#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class NetworkClient : public QObject {
    Q_OBJECT
public:
    explicit NetworkClient(QObject* parent = nullptr);

    void connectToServer(const QString& host, int port);
    void login(const QString& username, const QString& password);
    void registerUser(const QString& username, const QString& password);
    void sendMessage(const QString& to, const QString& content);
    void requestUserList();
    void requestHistory(const QString& with);
    void addContact(const QString& contact);
    void requestContacts();
    void requestAllUsers();  // 管理员

signals:
    void loginOk(const QString& username, const QString& role);
    void loginError(const QString& msg);
    void registerOk();
    void registerError(const QString& msg);
    void messageReceived(const QString& from, const QString& to,
                         const QString& content, const QString& time);
    void userListReceived(const QStringList& users);
    void contactsReceived(const QStringList& contacts);
    void historyReceived(const QString& with, const QJsonArray& messages);
    void allUsersReceived(const QJsonArray& users);
    void contactAdded(const QString& contact);
    void disconnected();

private slots:
    void onConnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError err);

private:
    void sendJson(const QJsonObject& obj);
    void processMessage(const QJsonObject& obj);

    QTcpSocket* socket_;
    QByteArray  readBuffer_;
};
