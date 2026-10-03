// 键盘快捷键管理器 - IntelNet 浏览器
#ifndef KEYBOARD_SHORTCUTS_H
#define KEYBOARD_SHORTCUTS_H

#include <QObject>
#include <QWidget>
#include <QKeySequence>
#include <QShortcut>
#include <QHash>
#include <QDialog>
#include <QTableWidget>
#include <QLabel>

namespace IntelNet {

/**
 * @brief 快捷键动作类型
 */
enum class ShortcutAction {
    // 语音相关
    ActivateVoice,          // Ctrl+Space
    StopVoice,              // Escape

    // 导航相关
    GoBack,                 // Alt+Left
    GoForward,              // Alt+Right
    Refresh,                // F5 / Ctrl+R
    Home,                   // Alt+Home
    FocusAddressBar,        // Ctrl+L / F6

    // 朗读控制
    ToggleReading,          // Ctrl+Shift+R
    PauseReading,           // Space (在朗读时)
    NextParagraph,          // Ctrl+Down
    PreviousParagraph,      // Ctrl+Up
    IncreaseSpeed,          // Ctrl+]
    DecreaseSpeed,          // Ctrl+[

    // AI 助手
    OpenAIAssistant,        // Ctrl+Shift+A
    QuickSummarize,         // Ctrl+S
    QuickTranslate,         // Ctrl+T

    // 阅读模式
    ToggleReadingMode,      // Ctrl+Shift+M
    IncreaseFontSize,       // Ctrl++
    DecreaseFontSize,       // Ctrl+-
    ResetFontSize,          // Ctrl+0

    // 快捷操作
    Bookmark,               // Ctrl+D
    History,                // Ctrl+H
    Downloads,              // Ctrl+J
    Settings,               // Ctrl+,

    // 页面操作
    SelectAll,              // Ctrl+A
    Copy,                   // Ctrl+C
    Find,                   // Ctrl+F
    Print,                  // Ctrl+P

    // 窗口管理
    NewTab,                 // Ctrl+T (暂不支持多标签)
    CloseWindow,            // Ctrl+W / Alt+F4
    Quit,                   // Ctrl+Q

    // 无障碍
    ToggleHighContrast,     // Ctrl+Shift+H
    ShowShortcutHelp,       // F1 / Ctrl+?
    ReadCurrentElement,     // Ctrl+Shift+Space
    DescribeImage,          // Ctrl+Shift+I

    // 调试
    OpenDevTools             // F12 (开发者工具)
};

/**
 * @brief 快捷键配置
 */
struct ShortcutConfig {
    ShortcutAction action;
    QKeySequence primaryKey;
    QKeySequence alternateKey;  // 可选的备用键
    QString description;
    QString category;  // 用于分组显示
    bool enabled;

    ShortcutConfig()
        : action(ShortcutAction::ActivateVoice)
        , enabled(true)
    {}

    ShortcutConfig(ShortcutAction act, const QKeySequence &primary,
                   const QString &desc, const QString &cat)
        : action(act)
        , primaryKey(primary)
        , description(desc)
        , category(cat)
        , enabled(true)
    {}

    ShortcutConfig(ShortcutAction act, const QKeySequence &primary,
                   const QKeySequence &alternate, const QString &desc, const QString &cat)
        : action(act)
        , primaryKey(primary)
        , alternateKey(alternate)
        , description(desc)
        , category(cat)
        , enabled(true)
    {}
};

/**
 * @brief 键盘快捷键管理器
 *
 * 功能：
 * - 集中管理所有快捷键
 * - 支持自定义快捷键
 * - 冲突检测
 * - 帮助文档生成
 */
class KeyboardShortcutManager : public QObject {
    Q_OBJECT

public:
    static KeyboardShortcutManager* instance();

    // 初始化快捷键
    void initialize(QWidget *parent);

    // 获取快捷键配置
    ShortcutConfig getConfig(ShortcutAction action) const;
    QList<ShortcutConfig> getAllConfigs() const;
    QList<ShortcutConfig> getConfigsByCategory(const QString &category) const;

    // 修改快捷键
    bool setShortcut(ShortcutAction action, const QKeySequence &newKey);
    bool hasConflict(const QKeySequence &key, ShortcutAction excludeAction = ShortcutAction::ActivateVoice) const;

    // 启用/禁用
    void enableShortcut(ShortcutAction action, bool enabled);
    bool isEnabled(ShortcutAction action) const;

    // 重置为默认
    void resetToDefaults();
    void resetAction(ShortcutAction action);

    // 导出/导入配置
    void saveToFile(const QString &filePath);
    void loadFromFile(const QString &filePath);

    // 帮助
    QString getShortcutText(ShortcutAction action) const;
    QString getCategoryName(const QString &category) const;

signals:
    // 快捷键触发
    void shortcutActivated(ShortcutAction action);

    // 配置变化
    void shortcutChanged(ShortcutAction action, const QKeySequence &oldKey, const QKeySequence &newKey);

private:
    explicit KeyboardShortcutManager(QObject *parent = nullptr);
    ~KeyboardShortcutManager() override;

    void setupDefaultShortcuts();
    void createShortcut(const ShortcutConfig &config);
    void removeShortcut(ShortcutAction action);

    static KeyboardShortcutManager *instance_;

    QWidget *parentWidget_;
    QHash<ShortcutAction, ShortcutConfig> configs_;
    QHash<ShortcutAction, QShortcut*> shortcuts_;
    QHash<QString, QString> categoryNames_;
};

/**
 * @brief 快捷键帮助对话框
 *
 * 显示所有可用快捷键的帮助
 */
class ShortcutHelpDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShortcutHelpDialog(QWidget *parent = nullptr);

    // 支持语音朗读
    void readCategory(const QString &category);
    void readAll();

signals:
    void speakRequested(const QString &text);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupUi();
    void populateShortcuts();
    QString formatShortcutList() const;

    QTableWidget *shortcutTable_;
    QPushButton *readAllButton_;
    QPushButton *closeButton_;
    QLabel *tipLabel_;
};

/**
 * @brief 快捷键冲突检测器
 */
class ShortcutConflictDetector {
public:
    struct Conflict {
        ShortcutAction action1;
        ShortcutAction action2;
        QKeySequence key;
    };

    static QList<Conflict> detectConflicts();
    static bool hasConflict(const QKeySequence &key,
                           const QHash<ShortcutAction, ShortcutConfig> &configs,
                           ShortcutAction excludeAction);
};

/**
 * @brief 快捷键提示工具
 *
 * 在 UI 上显示当前可用的快捷键提示
 */
class ShortcutHintOverlay : public QWidget {
    Q_OBJECT

public:
    explicit ShortcutHintOverlay(QWidget *parent = nullptr);

    void showHints(const QList<ShortcutAction> &actions);
    void hideHints();

    void setAutoHideDelay(int milliseconds);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();

    QList<ShortcutAction> currentActions_;
    QTimer *autoHideTimer_;
    QPropertyAnimation *fadeAnimation_;
    QGraphicsOpacityEffect *opacityEffect_;
};

} // namespace IntelNet

#endif // KEYBOARD_SHORTCUTS_H
