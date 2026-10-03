// AI 助手侧边栏 - IntelNet 浏览器
#ifndef AI_ASSISTANT_PANEL_H
#define AI_ASSISTANT_PANEL_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QListWidget>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QDateTime>

namespace IntelNet {

/**
 * @brief AI 建议卡片
 *
 * 显示单个智能建议
 */
class SuggestionCard : public QWidget {
    Q_OBJECT

public:
    enum SuggestionType {
        Summarize,      // 总结长文
        Translate,      // 翻译外语
        ReadingMode,    // 阅读模式
        ImageDescription, // 图片描述
        FormHelp,       // 表单帮助
        Custom          // 自定义
    };

    explicit SuggestionCard(SuggestionType type, const QString &text, QWidget *parent = nullptr);

    SuggestionType type() const { return type_; }
    QString text() const { return text_; }
    bool isRead() const { return isRead_; }
    void markAsRead();

signals:
    void accepted();
    void dismissed();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();
    QString getIconForType() const;
    QString getTitleForType() const;

    SuggestionType type_;
    QString text_;
    bool isRead_;

    QLabel *iconLabel_;
    QLabel *titleLabel_;
    QLabel *textLabel_;
    QPushButton *acceptButton_;
    QPushButton *dismissButton_;
};

/**
 * @brief 对话消息气泡
 *
 * 用户和 AI 的对话消息
 */
class MessageBubble : public QWidget {
    Q_OBJECT

public:
    enum Role {
        User,
        Assistant
    };

    explicit MessageBubble(Role role, const QString &message, const QDateTime &timestamp, QWidget *parent = nullptr);

    Role role() const { return role_; }
    QString message() const { return message_; }

    // 朗读此消息
    void speak();

signals:
    void speakRequested(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();

    Role role_;
    QString message_;
    QDateTime timestamp_;

    QLabel *messageLabel_;
    QLabel *timeLabel_;
    QPushButton *speakButton_;
};

/**
 * @brief 对话历史列表
 */
class ConversationHistory : public QScrollArea {
    Q_OBJECT

public:
    explicit ConversationHistory(QWidget *parent = nullptr);

    void addMessage(MessageBubble::Role role, const QString &message);
    void clear();
    int messageCount() const;

signals:
    void speakRequested(const QString &text);

private:
    void scrollToBottom();

    QWidget *contentWidget_;
    QVBoxLayout *layout_;
};

/**
 * @brief 智能建议列表
 */
class SuggestionList : public QWidget {
    Q_OBJECT

public:
    explicit SuggestionList(QWidget *parent = nullptr);

    void addSuggestion(SuggestionCard::SuggestionType type, const QString &text);
    void clearSuggestions();
    int unreadCount() const;

signals:
    void suggestionAccepted(SuggestionCard::SuggestionType type, const QString &text);
    void suggestionDismissed(SuggestionCard::SuggestionType type);
    void unreadCountChanged(int count);

private:
    void setupUi();
    void updateUnreadCount();

    QVBoxLayout *layout_;
    QList<SuggestionCard*> suggestions_;
};

/**
 * @brief AI 助手面板（主类）
 *
 * 侧边栏面板，包含：
 * - 智能建议
 * - 对话历史
 * - 输入框
 */
class AIAssistantPanel : public QWidget {
    Q_OBJECT

public:
    explicit AIAssistantPanel(QWidget *parent = nullptr);

    // 建议管理
    void addSuggestion(SuggestionCard::SuggestionType type, const QString &text);
    void clearSuggestions();
    int unreadSuggestionCount() const;

    // 对话管理
    void addUserMessage(const QString &message);
    void addAssistantMessage(const QString &message);
    void clearConversation();

    // 显示/隐藏
    void slideIn();
    void slideOut();
    bool isVisible() const;

signals:
    // 用户输入
    void userMessageSubmitted(const QString &message);

    // 建议操作
    void suggestionAccepted(SuggestionCard::SuggestionType type, const QString &text);
    void suggestionDismissed(SuggestionCard::SuggestionType type);

    // 未读数量变化
    void unreadCountChanged(int count);

    // 朗读请求
    void speakRequested(const QString &text);

private slots:
    void onSendButtonClicked();
    void onInputTextChanged();

private:
    void setupUi();
    void setupAnimations();
    void setupAccessibility();

    // UI 组件
    QLabel *headerLabel_;
    QPushButton *closeButton_;
    SuggestionList *suggestionList_;
    ConversationHistory *conversationHistory_;
    QTextEdit *inputEdit_;
    QPushButton *sendButton_;
    QPushButton *speakButton_;

    // 布局
    QVBoxLayout *mainLayout_;

    // 动画
    QPropertyAnimation *slideAnimation_;
    bool isShown_;
};

/**
 * @brief 上下文感知的建议生成器
 *
 * 根据页面内容自动生成智能建议
 */
class SmartSuggestionGenerator : public QObject {
    Q_OBJECT

public:
    explicit SmartSuggestionGenerator(QObject *parent = nullptr);

    // 分析页面内容并生成建议
    void analyzePage(const QString &html, const QString &url);

    // 检测特定内容
    bool hasLongText(const QString &html) const;
    bool hasForeignLanguage(const QString &html) const;
    bool hasForm(const QString &html) const;
    bool hasVideo(const QString &html) const;
    bool hasImages(const QString &html) const;

signals:
    void suggestionGenerated(SuggestionCard::SuggestionType type, const QString &text);

private:
    int estimateReadingTime(const QString &text) const;
    QString detectLanguage(const QString &text) const;
    int countWords(const QString &text) const;
};

} // namespace IntelNet

#endif // AI_ASSISTANT_PANEL_H
