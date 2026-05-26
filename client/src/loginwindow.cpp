#include "loginwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

// WeChat 风格输入框
static QLineEdit* makeInput(const QString& placeholder, bool password = false) {
    auto* e = new QLineEdit();
    e->setPlaceholderText(placeholder);
    e->setMinimumHeight(40);
    if (password) e->setEchoMode(QLineEdit::Password);
    e->setStyleSheet(
        "QLineEdit { border: 1px solid #ddd; border-radius: 4px; padding: 0 12px;"
        "font-size: 14px; background: white; }"
        "QLineEdit:focus { border-color: #07C160; }");
    return e;
}

LoginWindow::LoginWindow(NetworkClient* net, QWidget* parent)
    : QDialog(parent), net_(net)
{
    setWindowTitle("WeChat");
    setFixedSize(360, 340);
    setStyleSheet("QDialog { background: #F5F5F5; }");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 20, 40, 20);

    stack_ = new QStackedWidget();
    mainLayout->addWidget(stack_);

    // ====== 登录页 ======
    auto* loginPage = new QWidget();
    auto* ll = new QVBoxLayout(loginPage);
    ll->setSpacing(12);

    auto* title1 = new QLabel("登录");
    title1->setAlignment(Qt::AlignCenter);
    title1->setStyleSheet("font-size: 20px; font-weight: bold; color: #333;");
    ll->addWidget(title1);

    loginUser_ = makeInput("用户名");
    ll->addWidget(loginUser_);

    loginPass_ = makeInput("密码", true);
    ll->addWidget(loginPass_);

    rememberBox_ = new QCheckBox("记住登录");
    rememberBox_->setStyleSheet("color: #666;");
    ll->addWidget(rememberBox_);

    loginBtn_ = new QPushButton("登 录");
    loginBtn_->setMinimumHeight(40);
    loginBtn_->setStyleSheet(
        "QPushButton { background: #07C160; color: white; border: none;"
        "border-radius: 4px; font-size: 15px; font-weight: bold; }"
        "QPushButton:hover { background: #06AD56; }"
        "QPushButton:pressed { background: #059048; }");
    ll->addWidget(loginBtn_);

    loginStatus_ = new QLabel();
    loginStatus_->setAlignment(Qt::AlignCenter);
    loginStatus_->setStyleSheet("color: #F44336; font-size: 12px;");
    ll->addWidget(loginStatus_);

    auto* regLink = new QPushButton("没有账号？注册");
    regLink->setFlat(true);
    regLink->setStyleSheet("color: #07C160; border: none; font-size: 13px;");
    ll->addWidget(regLink);

    stack_->addWidget(loginPage);

    // ====== 注册页 ======
    auto* regPage = new QWidget();
    auto* rl = new QVBoxLayout(regPage);
    rl->setSpacing(12);

    auto* title2 = new QLabel("注册");
    title2->setAlignment(Qt::AlignCenter);
    title2->setStyleSheet("font-size: 20px; font-weight: bold; color: #333;");
    rl->addWidget(title2);

    regUser_ = makeInput("用户名");
    rl->addWidget(regUser_);

    regPass_ = makeInput("密码", true);
    rl->addWidget(regPass_);

    regPass2_ = makeInput("确认密码", true);
    rl->addWidget(regPass2_);

    regBtn_ = new QPushButton("注 册");
    regBtn_->setMinimumHeight(40);
    regBtn_->setStyleSheet(
        "QPushButton { background: #07C160; color: white; border: none;"
        "border-radius: 4px; font-size: 15px; font-weight: bold; }"
        "QPushButton:hover { background: #06AD56; }");
    rl->addWidget(regBtn_);

    regStatus_ = new QLabel();
    regStatus_->setAlignment(Qt::AlignCenter);
    regStatus_->setStyleSheet("color: #F44336; font-size: 12px;");
    rl->addWidget(regStatus_);

    auto* loginLink = new QPushButton("已有账号？登录");
    loginLink->setFlat(true);
    loginLink->setStyleSheet("color: #07C160; border: none; font-size: 13px;");
    rl->addWidget(loginLink);

    stack_->addWidget(regPage);

    // 恢复记住的账号
    QSettings settings("WeChat", "ChatClient");
    QString savedUser = settings.value("lastUser").toString();
    if (!savedUser.isEmpty()) {
        loginUser_->setText(savedUser);
        rememberBox_->setChecked(true);
    }

    // 信号
    connect(loginBtn_, &QPushButton::clicked, this, &LoginWindow::onLogin);
    connect(loginPass_, &QLineEdit::returnPressed, this, &LoginWindow::onLogin);
    connect(regBtn_, &QPushButton::clicked, this, &LoginWindow::onRegister);
    connect(loginLink, &QPushButton::clicked, this, &LoginWindow::switchToLogin);
    connect(regLink, &QPushButton::clicked, this, &LoginWindow::switchToRegister);

    connect(net_, &NetworkClient::loginOk, this, [this](const QString& name) {
        username_ = name;
        if (rememberBox_->isChecked()) {
            QSettings s("WeChat", "ChatClient");
            s.setValue("lastUser", loginUser_->text());
        } else {
            QSettings s("WeChat", "ChatClient");
            s.remove("lastUser");
        }
        accept();
    });
    connect(net_, &NetworkClient::loginError, this, [this](const QString& msg) {
        loginStatus_->setText(msg);
        loginBtn_->setEnabled(true);
    });
    connect(net_, &NetworkClient::registerOk, this, [this]() {
        regStatus_->setStyleSheet("color: #07C160; font-size: 12px;");
        regStatus_->setText("注册成功！请登录");
        QTimer::singleShot(1000, this, &LoginWindow::switchToLogin);
    });
    connect(net_, &NetworkClient::registerError, this, [this](const QString& msg) {
        regStatus_->setText(msg);
        regBtn_->setEnabled(true);
    });
}

void LoginWindow::onLogin() {
    QString user = loginUser_->text().trimmed();
    QString pass = loginPass_->text();
    if (user.isEmpty()) { loginStatus_->setText("请输入用户名"); return; }
    loginBtn_->setEnabled(false);
    loginStatus_->setText("连接中...");
    net_->login(user, pass);
}

void LoginWindow::onRegister() {
    QString user = regUser_->text().trimmed();
    QString pass = regPass_->text();
    QString pass2 = regPass2_->text();
    if (user.isEmpty() || pass.isEmpty()) { regStatus_->setText("请填写完整"); return; }
    if (pass != pass2) { regStatus_->setText("两次密码不一致"); return; }
    regBtn_->setEnabled(false);
    regStatus_->setText("注册中...");
    net_->registerUser(user, pass);
}

void LoginWindow::switchToLogin()   { stack_->setCurrentIndex(0); }
void LoginWindow::switchToRegister() { stack_->setCurrentIndex(1); }
