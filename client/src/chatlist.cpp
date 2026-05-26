#include "chatlist.h"
#include <QHBoxLayout>

ChatList::ChatList(QWidget* parent) : QWidget(parent) {
    setFixedWidth(260);
    setStyleSheet("background: #2E2E2E;");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 搜索框
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("搜索");
    searchEdit_->setMinimumHeight(38);
    searchEdit_->setStyleSheet(
        "QLineEdit { background: #3A3A3A; border: none; border-radius: 4px;"
        "color: #BBB; padding: 0 12px; margin: 8px; font-size: 13px; }");
    layout->addWidget(searchEdit_);

    // 用户列表
    list_ = new QListWidget();
    list_->setStyleSheet(
        "QListWidget { background: #2E2E2E; border: none; color: #CCC;"
        "font-size: 14px; outline: none; }"
        "QListWidget::item { padding: 12px 16px; border-bottom: 1px solid #3A3A3A; }"
        "QListWidget::item:hover { background: #3A3A3A; }"
        "QListWidget::item:selected { background: #3A3A3A; color: #4CAF50; }");
    layout->addWidget(list_);

    connect(list_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        emit userSelected(item->text());
    });
}

void ChatList::addUser(const QString& username) {
    // 去重
    for (int i = 0; i < list_->count(); ++i) {
        if (list_->item(i)->text() == username) return;
    }
    auto* item = new QListWidgetItem(username);
    item->setSizeHint(QSize(0, 48));
    list_->insertItem(0, item);
}

void ChatList::showNotification(const QString& from) {
    for (int i = 0; i < list_->count(); ++i) {
        if (list_->item(i)->text() == from) {
            list_->item(i)->setForeground(QColor("#07C160"));
            // 移到顶部
            auto* item = list_->takeItem(i);
            list_->insertItem(0, item);
            return;
        }
    }
    // 新用户
    addUser(from);
}
