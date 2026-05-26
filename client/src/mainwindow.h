#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QTimer>
#include <QPushButton>
#include <QLineEdit>
#include <QJsonArray>
#include "networkclient.h"
#include "chatlist.h"
#include "chatview.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString& username, const QString& role,
                        NetworkClient* net, QWidget* parent = nullptr);

private slots:
    void onAddContact();

private:
    void loadContacts();
    void onContactSelected(const QString& username);

    NetworkClient* net_;
    ChatList*  chatList_;
    ChatView*  chatView_;
    QString    myName_;
    QString    myRole_;
    QPushButton* addContactBtn_;
    QLineEdit*   contactEdit_;
};
