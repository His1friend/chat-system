#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QMap>

struct ChatMessage {
    QString from;
    QString content;
    QString time;
    bool    isMine;
};

class ChatView : public QWidget {
    Q_OBJECT
public:
    explicit ChatView(QWidget* parent = nullptr);

    void setPeer(const QString& peer);
    void addMessage(const QString& from, const QString& content,
                    const QString& time, bool isMine);
    void clearMessages();

signals:
    void sendMessage(const QString& to, const QString& content);

private slots:
    void onSend();

private:
    void addBubble(const ChatMessage& msg);

    QLabel*    titleLabel_;
    QLabel*    peerLabel_;
    QWidget*   msgContainer_;
    QVBoxLayout* msgLayout_;
    QScrollArea* scrollArea_;
    QTextEdit*  inputEdit_;
    QPushButton* sendBtn_;

    QString    currentPeer_;
    QString    myName_;
    QMap<QString, QVector<ChatMessage>> history_;

public:
    void setMyName(const QString& name) { myName_ = name; }
};
