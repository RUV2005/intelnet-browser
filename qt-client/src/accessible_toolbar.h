// 无障碍顶部工具栏 - IntelNet 浏览器
#ifndef ACCESSIBLE_TOOLBAR_H
#define ACCESSIBLE_TOOLBAR_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

namespace IntelNet {

/**
 * @brief 大尺寸语音按钮（56x56px）
 *
 * 特点：
 * - 圆形设计，易于识别
 * - 三种状态：静止、聆听、识别中
 * - 支持按住说话和点击切换两种模式
 */
class VoiceButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(qreal pulseOpacity READ pulseOpacity WRITE setPulseOpacity)

public:
    enum State {
        Idle,       // 静止
        Listening,  // 正在聆听
        Processing  // 正在识别
    };

    explicit VoiceButton(QWidget *parent = nullptr);

    void setState(State state);
    State state() const { return state_; }

    qreal pulseOpacity() const { return pulseOpacity_; }
    void setPulseOpacity(qreal opacity);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void pressAndHold();  // 按住说话
    void toggleMode();    // 切换模式

private:
    void startPulseAnimation();
    void stopPulseAnimation();

    State state_;
    qreal pulseOpacity_;
    bool isHovered_;
    QPropertyAnimation *pulseAnimation_;
};

/**
 * @brief 智能状态栏
 *
 * 显示当前页面信息而非原始 URL
 * - 页面标题（AI 提取）
 * - 来源/作者
 * - 阅读时长估计
 */
class SmartStatusBar : public QWidget {
    Q_OBJECT

public:
    explicit SmartStatusBar(QWidget *parent = nullptr);

    void setPageTitle(const QString &title);
    void setSource(const QString &source);
    void setReadingTime(int minutes);
    void setCurrentStatus(const QString &status);

private:
    void setupUi();

    QLabel *titleLabel_;
    QLabel *metaLabel_;
    QLabel *statusLabel_;
};

/**
 * @brief 朗读控制条（仅朗读时显示）
 *
 * 功能：
 * - 进度显示（按段落）
 * - 播放控制
 * - 速度调节
 */
class ReadingControlBar : public QWidget {
    Q_OBJECT

public:
    explicit ReadingControlBar(QWidget *parent = nullptr);

    void setProgress(int current, int total);
    void setSpeed(qreal speed);
    void setPlaying(bool playing);

signals:
    void previousParagraph();
    void togglePlayPause();
    void nextParagraph();
    void speedChanged(qreal speed);
    void seekToPosition(int paragraph);

private:
    void setupUi();

    QProgressBar *progressBar_;
    QLabel *positionLabel_;
    QPushButton *prevButton_;
    QPushButton *playPauseButton_;
    QPushButton *nextButton_;
    QPushButton *speedButton_;

    int currentParagraph_;
    int totalParagraphs_;
    qreal currentSpeed_;
    bool isPlaying_;
};

/**
 * @brief AI 助手入口按钮
 *
 * 显示未读建议数量
 */
class AIAssistantButton : public QPushButton {
    Q_OBJECT

public:
    explicit AIAssistantButton(QWidget *parent = nullptr);

    void setUnreadCount(int count);
    int unreadCount() const { return unreadCount_; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int unreadCount_;
};

/**
 * @brief 快捷操作栏（可折叠）
 *
 * 包含：阅读模式、总结、翻译、收藏等
 */
class QuickActionBar : public QWidget {
    Q_OBJECT

public:
    explicit QuickActionBar(QWidget *parent = nullptr);

    void expand();
    void collapse();
    bool isExpanded() const { return isExpanded_; }

signals:
    void readingModeClicked();
    void summarizeClicked();
    void translateClicked();
    void bookmarkClicked();

private:
    void setupUi();
    void setupAnimation();

    QPushButton *readingModeButton_;
    QPushButton *summarizeButton_;
    QPushButton *translateButton_;
    QPushButton *bookmarkButton_;

    bool isExpanded_;
    QPropertyAnimation *expandAnimation_;
    QGraphicsOpacityEffect *opacityEffect_;
};

/**
 * @brief 无障碍顶部工具栏（主类）
 *
 * 沉浸式、自适应设计
 * - 默认简洁模式
 * - 按需展开详细控制
 * - 完整键盘导航支持
 */
class AccessibleToolbar : public QWidget {
    Q_OBJECT

public:
    explicit AccessibleToolbar(QWidget *parent = nullptr);

    // 状态管理
    void setPageInfo(const QString &title, const QString &source, int readingMinutes);
    void setReadingProgress(int current, int total);
    void setReadingSpeed(qreal speed);
    void setReadingState(bool playing);
    void setAIUnreadCount(int count);

    // 模式切换
    void showReadingControls();
    void hideReadingControls();
    void expandQuickActions();
    void collapseQuickActions();

signals:
    // 语音相关
    void voicePressAndHold();
    void voiceToggleMode();

    // 导航相关
    void backRequested();
    void forwardRequested();
    void refreshRequested();

    // 朗读控制
    void readingPrevious();
    void readingTogglePlayPause();
    void readingNext();
    void readingSpeedChanged(qreal speed);
    void readingSeekTo(int paragraph);

    // AI 助手
    void aiAssistantRequested();

    // 快捷操作
    void readingModeRequested();
    void summarizeRequested();
    void translateRequested();
    void bookmarkRequested();

    // 设置和菜单
    void settingsRequested();
    void menuRequested();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupUi();
    void setupConnections();
    void setupAccessibility();
    void setupKeyboardNavigation();
    void setupAutoCollapse();

    // UI 组件
    VoiceButton *voiceButton_;
    SmartStatusBar *statusBar_;
    ReadingControlBar *readingBar_;
    AIAssistantButton *aiButton_;
    QuickActionBar *quickActions_;

    QPushButton *backButton_;
    QPushButton *forwardButton_;
    QPushButton *refreshButton_;
    QPushButton *settingsButton_;
    QPushButton *menuButton_;

    // 布局
    QVBoxLayout *mainLayout_;
    QHBoxLayout *topRowLayout_;
    QHBoxLayout *navigationLayout_;

    // 自动折叠计时器
    QTimer *collapseTimer_;
    bool isExpanded_;
};

} // namespace IntelNet

#endif // ACCESSIBLE_TOOLBAR_H
