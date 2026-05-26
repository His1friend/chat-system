#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QStackedWidget>
#include <QSettings>
#include <QTimer>
#include "networkclient.h"

class LoginWindow : public QDialog {
    Q_OBJECT
public:
    explicit LoginWindow(NetworkClient* net, QWidget* parent = nullptr);
    QString username() const { return username_; }

private slots:
    void onLogin();
    void onRegister();
    void switchToLogin();
    void switchToRegister();

private:
    NetworkClient* net_;
    QStackedWidget* stack_;

    // 登录页
    QLineEdit* loginUser_;
    QLineEdit* loginPass_;
    QCheckBox* rememberBox_;
    QPushButton* loginBtn_;
    QLabel* loginStatus_;

    // 注册页
    QLineEdit* regUser_;
    QLineEdit* regPass_;
    QLineEdit* regPass2_;
    QPushButton* regBtn_;
    QLabel* regStatus_;

    QString username_;
};
