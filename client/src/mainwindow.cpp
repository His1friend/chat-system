#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonArray>

MainWindow::MainWindow(const QString& username, const QString& role,
                       NetworkClient* net, QWidget* parent)
    : QMainWindow(parent), net_(net), myName_(username), myRole_(role)
{
    setWindowTitle(QString("WeChat — %1%2")
                       .arg(username, role == "admin" ? " [管理员]" : ""));
    resize(960, 640);
    setMinimumSize(800, 500);

    setStyleSheet("QMainWindow { background: #F5F5F5; }");

    auto* central = new QWidget();
    setCentralWidget(central);
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 左侧
    auto* leftPanel = new QWidget();
    leftPanel->setFixedWidth(260);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    // 添加联系人栏
    auto* addBar = new QWidget();
    addBar->setFixedHeight(44);
    addBar->setStyleSheet("background: #2E2E2E;");
    auto* addLayout = new QHBoxLayout(addBar);
    addLayout->setContentsMargins(8, 6, 8, 6);

    contactEdit_ = new QLineEdit();
    contactEdit_->setPlaceholderText("添加联系人...");
    contactEdit_->setStyleSheet(
        "QLineEdit { background: #3A3A3A; border: none; border-radius: 4px;"
        "color: #BBB; padding: 0 8px; font-size: 12px; }");
    addLayout->addWidget(contactEdit_);

    addContactBtn_ = new QPushButton("+");
    addContactBtn_->setFixedSize(32, 28);
    addContactBtn_->setStyleSheet(
        "QPushButton { background: #07C160; color: white; border: none;"
        "border-radius: 4px; font-size: 18px; font-weight: bold; }"
        "QPushButton:hover { background: #06AD56; }");
    addLayout->addWidget(addContactBtn_);

    leftLayout->addWidget(addBar);

    chatList_ = new ChatList();
    leftLayout->addWidget(chatList_);

    layout->addWidget(leftPanel);

    // 分割线
    auto* div = new QWidget(); div->setFixedWidth(1);
    div->setStyleSheet("background: #D0D0D0;");
    layout->addWidget(div);

    // 右侧
    chatView_ = new ChatView();
    chatView_->setMyName(username);
    layout->addWidget(chatView_, 1);

    // ---- 信号 ----
    connect(addContactBtn_, &QPushButton::clicked, this, &MainWindow::onAddContact);
    connect(contactEdit_, &QLineEdit::returnPressed, this, &MainWindow::onAddContact);

    connect(chatList_, &ChatList::userSelected, this, &MainWindow::onContactSelected);

    connect(chatView_, &ChatView::sendMessage, this, [this](const QString& to, const QString& content) {
        net_->sendMessage(to, content);
    });

    connect(net_, &NetworkClient::messageReceived, this,
            [this](const QString& from, const QString& to, const QString& content, const QString& time) {
        bool isMine = (from == myName_);
        QString peer = isMine ? to : from;
        chatView_->addMessage(from, content, time, isMine);
        if (!isMine) chatList_->showNotification(from);
    });

    connect(net_, &NetworkClient::userListReceived, this, [this](const QStringList& users) {
        for (auto& u : users)
            if (u != myName_) chatList_->addUser(u);
    });

    connect(net_, &NetworkClient::contactsReceived, this, [this](const QStringList& contacts) {
        for (auto& c : contacts) chatList_->addUser(c);
    });

    connect(net_, &NetworkClient::contactAdded, this, [this](const QString& contact) {
        chatList_->addUser(contact);
    });

    connect(net_, &NetworkClient::historyReceived, this,
            [this](const QString&, const QJsonArray& msgs) {
        for (int i = msgs.size() - 1; i >= 0; --i) {
            auto m = msgs[i].toObject();
            chatView_->addMessage(m["from"].toString(), m["content"].toString(),
                                   m["time"].toString(),
                                   m["from"].toString() == myName_);
        }
    });

    // 管理员功能
    if (role == "admin") {
        connect(net_, &NetworkClient::allUsersReceived, this,
                [this](const QJsonArray& users) {
            for (auto v : users) {
                auto u = v.toObject();
                QString name = u["name"].toString();
                if (name != myName_) chatList_->addUser(name);
            }
        });
    }

    // 定时刷新
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() { loadContacts(); });
    timer->start(3000);

    // 初始加载
    QTimer::singleShot(500, this, [this]() { loadContacts(); });
}

void MainWindow::loadContacts() {
    if (myRole_ == "admin") net_->requestAllUsers();
    else net_->requestContacts();
}

void MainWindow::onAddContact() {
    QString name = contactEdit_->text().trimmed();
    if (name.isEmpty() || name == myName_) return;
    net_->addContact(name);
    contactEdit_->clear();
}

void MainWindow::onContactSelected(const QString& username) {
    chatView_->setPeer(username);
    net_->requestHistory(username);
}
