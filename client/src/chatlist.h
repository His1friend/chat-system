#pragma once

#include <QWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>

class ChatList : public QWidget {
    Q_OBJECT
public:
    explicit ChatList(QWidget* parent = nullptr);

    void addUser(const QString& username);
    void showNotification(const QString& from);

signals:
    void userSelected(const QString& username);

private:
    QListWidget* list_;
    QLineEdit*   searchEdit_;
};
