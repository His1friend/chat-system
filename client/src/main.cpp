#include <QApplication>
#include <QTimer>
#include "networkclient.h"
#include "loginwindow.h"
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("WeChat");
    app.setStyle("Fusion");

    auto* net = new NetworkClient();
    net->connectToServer("127.0.0.1", 9000);

    QString username;
    QString role = "user";

    QObject::connect(net, &NetworkClient::loginOk,
                     [&](const QString& name, const QString& r) {
        username = name;
        role = r;
    });

    LoginWindow login(net);
    if (login.exec() != QDialog::Accepted || username.isEmpty()) {
        delete net;
        return 0;
    }

    auto* mainWin = new MainWindow(username, role, net);
    mainWin->setAttribute(Qt::WA_DeleteOnClose);
    mainWin->show();

    return app.exec();
}
