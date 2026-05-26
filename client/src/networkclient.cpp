#include "networkclient.h"
#include <QHostAddress>
#include <cstring>

NetworkClient::NetworkClient(QObject* parent) : QObject(parent) {
    socket_ = new QTcpSocket(this);
    connect(socket_, &QTcpSocket::connected, this, &NetworkClient::onConnected);
    connect(socket_, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(socket_, &QTcpSocket::errorOccurred, this, &NetworkClient::onError);
}

void NetworkClient::connectToServer(const QString& host, int port) {
    socket_->connectToHost(host, port);
}

void NetworkClient::login(const QString& username, const QString& password) {
    QJsonObject obj;
    obj["type"] = "login";
    obj["username"] = username;
    obj["password"] = password;
    sendJson(obj);
}

void NetworkClient::registerUser(const QString& username, const QString& password) {
    QJsonObject obj;
    obj["type"] = "register";
    obj["username"] = username;
    obj["password"] = password;
    sendJson(obj);
}

void NetworkClient::sendMessage(const QString& to, const QString& content) {
    QJsonObject obj;
    obj["type"] = "msg";
    obj["to"] = to;
    obj["content"] = content;
    sendJson(obj);
}

void NetworkClient::requestUserList() {
    QJsonObject obj; obj["type"] = "list"; sendJson(obj);
}

void NetworkClient::requestHistory(const QString& with) {
    QJsonObject obj;
    obj["type"] = "history";
    obj["with"] = with;
    sendJson(obj);
}

void NetworkClient::addContact(const QString& contact) {
    QJsonObject obj;
    obj["type"] = "add_contact";
    obj["contact"] = contact;
    sendJson(obj);
}

void NetworkClient::requestContacts() {
    QJsonObject obj; obj["type"] = "get_contacts"; sendJson(obj);
}

void NetworkClient::requestAllUsers() {
    QJsonObject obj; obj["type"] = "admin_users"; sendJson(obj);
}

void NetworkClient::sendJson(const QJsonObject& obj) {
    QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    uint32_t len = json.size();
    QByteArray packet(4, '\0');
    memcpy(packet.data(), &len, 4);
    packet.append(json);
    socket_->write(packet);
}

void NetworkClient::onConnected() {}

void NetworkClient::onReadyRead() {
    readBuffer_.append(socket_->readAll());
    while (readBuffer_.size() >= 4) {
        uint32_t len; memcpy(&len, readBuffer_.constData(), 4);
        if (static_cast<uint32_t>(readBuffer_.size()) < 4 + len) break;
        QByteArray data = readBuffer_.mid(4, len);
        readBuffer_.remove(0, 4 + len);
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) processMessage(doc.object());
    }
}

void NetworkClient::processMessage(const QJsonObject& obj) {
    QString type = obj["type"].toString();

    if (type == "login_ok")
        emit loginOk(obj["username"].toString(), obj["role"].toString());
    else if (type == "register_ok")
        emit registerOk();
    else if (type == "error") {
        // 分发错误类型
        QString msg = obj["msg"].toString();
        if (msg.contains("用户名或密码") || msg.contains("密码错误"))
            emit loginError(msg);
        else
            emit registerError(msg);
    } else if (type == "msg")
        emit messageReceived(obj["from"].toString(), obj["to"].toString(),
                             obj["content"].toString(), obj["time"].toString());
    else if (type == "user_list") {
        QStringList users;
        for (auto v : obj["users"].toArray()) {
            QJsonObject u = v.toObject();
            users << u["name"].toString();
        }
        emit userListReceived(users);
    } else if (type == "contacts") {
        QStringList contacts;
        for (auto v : obj["contacts"].toArray())
            contacts << v.toString();
        emit contactsReceived(contacts);
    } else if (type == "history")
        emit historyReceived(obj["with"].toString(), obj["messages"].toArray());
    else if (type == "contact_added")
        emit contactAdded(obj["contact"].toString());
    else if (type == "all_users")
        emit allUsersReceived(obj["users"].toArray());
    else if (type == "kicked")
        emit disconnected();
}

void NetworkClient::onError(QAbstractSocket::SocketError) {
    emit loginError("连接失败: " + socket_->errorString());
}
