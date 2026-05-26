#include "chatview.h"
#include <QHBoxLayout>
#include <QScrollBar>
#include <QDateTime>
#include <QTimer>

ChatView::ChatView(QWidget* parent) : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---- 顶部标题栏 ----
    auto* topBar = new QWidget();
    topBar->setFixedHeight(56);
    topBar->setStyleSheet("background: #F5F5F5; border-bottom: 1px solid #E0E0E0;");
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(20, 0, 20, 0);

    titleLabel_ = new QLabel("选择联系人开始聊天");
    titleLabel_->setStyleSheet("font-size: 17px; font-weight: bold; color: #333;");
    topLayout->addWidget(titleLabel_);
    topLayout->addStretch();
    mainLayout->addWidget(topBar);

    // ---- 消息区域（可滚动） ----
    scrollArea_ = new QScrollArea();
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setStyleSheet("QScrollArea { border: none; background: #F5F5F5; }");
    scrollArea_->verticalScrollBar()->setStyleSheet(
        "QScrollBar:vertical { width: 6px; background: transparent; }"
        "QScrollBar::handle:vertical { background: #CCC; border-radius: 3px; }");

    msgContainer_ = new QWidget();
    msgContainer_->setStyleSheet("background: #F5F5F5;");
    msgLayout_ = new QVBoxLayout(msgContainer_);
    msgLayout_->setContentsMargins(16, 12, 16, 12);
    msgLayout_->setSpacing(12);
    msgLayout_->addStretch();

    scrollArea_->setWidget(msgContainer_);
    mainLayout->addWidget(scrollArea_, 1);

    // ---- 底部输入区域 ----
    auto* bottomBar = new QWidget();
    bottomBar->setFixedHeight(160);
    bottomBar->setStyleSheet("background: #F5F5F5; border-top: 1px solid #E0E0E0;");
    auto* bottomLayout = new QVBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(16, 10, 16, 10);
    bottomLayout->setSpacing(8);

    inputEdit_ = new QTextEdit();
    inputEdit_->setPlaceholderText("输入消息...");
    inputEdit_->setMaximumHeight(80);
    inputEdit_->setStyleSheet(
        "QTextEdit { border: none; background: white; border-radius: 4px;"
        "padding: 8px; font-size: 14px; }");
    bottomLayout->addWidget(inputEdit_);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    sendBtn_ = new QPushButton("发送");
    sendBtn_->setFixedSize(70, 32);
    sendBtn_->setStyleSheet(
        "QPushButton { background: #07C160; color: white; border: none;"
        "border-radius: 4px; font-size: 13px; }"
        "QPushButton:hover { background: #06AD56; }");
    btnRow->addWidget(sendBtn_);
    bottomLayout->addLayout(btnRow);

    mainLayout->addWidget(bottomBar);

    connect(sendBtn_, &QPushButton::clicked, this, &ChatView::onSend);
    connect(inputEdit_, &QTextEdit::textChanged, this, [this]() {
        // Ctrl+Enter 发送
    });
    // Enter 发送快捷键
    inputEdit_->installEventFilter(this);
}

void ChatView::setPeer(const QString& peer) {
    if (peer == currentPeer_) return;

    // 保存当前历史
    if (!currentPeer_.isEmpty()) {
        // 消息已在 addMessage 中实时保存
    }

    currentPeer_ = peer;
    titleLabel_->setText(peer);

    // 清除并恢复历史
    clearMessages();
    if (history_.contains(peer)) {
        for (auto& m : history_[peer]) {
            addBubble(m);
        }
    }
}

void ChatView::addMessage(const QString& from, const QString& content,
                           const QString& time, bool isMine) {
    ChatMessage msg{from, content, time, isMine};

    QString peer = isMine ? from : from;
    history_[peer].append(msg);

    if (peer == currentPeer_ || (isMine && from == currentPeer_)) {
        addBubble(msg);
    }
}

void ChatView::addBubble(const ChatMessage& msg) {
    auto* bubble = new QWidget();
    auto* bl = new QHBoxLayout(bubble);
    bl->setContentsMargins(0, 2, 0, 2);

    auto* textLabel = new QLabel(msg.content);
    textLabel->setWordWrap(true);
    textLabel->setMaximumWidth(360);
    textLabel->setContentsMargins(12, 8, 12, 8);

    QString bubbleStyle;
    QString alignStyle;

    if (msg.isMine) {
        // 我的消息：右对齐，绿色气泡
        bubbleStyle = "background: #95EC69; border-radius: 6px;";
        alignStyle = "margin-left: 80px;";
        bl->addStretch();
        bl->addWidget(textLabel);
    } else {
        // 对方的消息：左对齐，白色气泡
        bubbleStyle = "background: white; border-radius: 6px;";
        bl->addWidget(textLabel);
        bl->addStretch();
    }

    textLabel->setStyleSheet(
        QString("QLabel { %1 font-size: 14px; color: #333; padding: 10px 14px; }"
                "QLabel { %2 }").arg(bubbleStyle, alignStyle));

    // 插入到 stretch 之前（stretch 是最后一个元素）
    int insertPos = msgLayout_->count() - 1;
    msgLayout_->insertWidget(insertPos, bubble);

    // 滚动到底部
    QTimer::singleShot(50, [this]() {
        scrollArea_->verticalScrollBar()->setValue(
            scrollArea_->verticalScrollBar()->maximum());
    });
}

void ChatView::clearMessages() {
    // 移除除 stretch 外的所有 widget
    while (msgLayout_->count() > 1) {
        auto* item = msgLayout_->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void ChatView::onSend() {
    QString text = inputEdit_->toPlainText().trimmed();
    if (text.isEmpty() || currentPeer_.isEmpty()) return;

    inputEdit_->clear();

    QString time = QDateTime::currentDateTime().toString("HH:mm");
    addMessage(currentPeer_, text, time, true);

    emit sendMessage(currentPeer_, text);
}
