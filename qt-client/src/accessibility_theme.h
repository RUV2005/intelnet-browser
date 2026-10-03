// 无障碍主题系统 - IntelNet 浏览器
#ifndef ACCESSIBILITY_THEME_H
#define ACCESSIBILITY_THEME_H

#include <QObject>
#include <QString>
#include <QColor>
#include <QFont>
#include <QHash>

namespace IntelNet {

/**
 * @brief 主题模式
 */
enum class ThemeMode {
    Standard,           // 标准模式
    HighContrast,       // 高对比度
    ExtraHighContrast,  // 超高对比度（WCAG AAA）
    Dark,               // 深色模式
    DarkHighContrast,   // 深色高对比度
    Custom              // 自定义
};

/**
 * @brief 字体大小预设
 */
enum class FontSizePreset {
    ExtraSmall,  // 10pt
    Small,       // 12pt
    Medium,      // 14pt (默认)
    Large,       // 18pt
    ExtraLarge,  // 24pt
    Huge         // 32pt
};

/**
 * @brief 对比度级别
 */
enum class ContrastLevel {
    Normal,      // 4.5:1 (WCAG AA)
    High,        // 7:1 (WCAG AAA)
    ExtraHigh    // 10:1+
};

/**
 * @brief 颜色方案
 */
struct ColorScheme {
    // 背景色
    QColor primaryBackground;
    QColor secondaryBackground;
    QColor surfaceBackground;

    // 文字色
    QColor primaryText;
    QColor secondaryText;
    QColor disabledText;

    // 强调色
    QColor accent;
    QColor accentHover;
    QColor accentPressed;

    // 状态色
    QColor success;
    QColor warning;
    QColor error;
    QColor info;

    // 边框色
    QColor border;
    QColor borderHover;
    QColor borderFocus;

    // 焦点指示器
    QColor focusIndicator;
    int focusIndicatorWidth;

    // 获取对比度
    double getContrast(const QColor &fg, const QColor &bg) const;
    bool meetsWCAG_AA(const QColor &fg, const QColor &bg) const;
    bool meetsWCAG_AAA(const QColor &fg, const QColor &bg) const;
};

/**
 * @brief 字体配置
 */
struct FontConfig {
    QString family;
    int baseSize;       // 基础大小（pt）
    int h1Size;         // 标题 1
    int h2Size;         // 标题 2
    int h3Size;         // 标题 3
    int buttonSize;     // 按钮
    int inputSize;      // 输入框
    int captionSize;    // 说明文字

    bool bold;
    int letterSpacing;  // 字间距（像素）
    int lineHeight;     // 行高（百分比）

    // 应用字体大小预设
    void applyPreset(FontSizePreset preset);
};

/**
 * @brief 主题配置
 */
struct ThemeConfig {
    ThemeMode mode;
    ColorScheme colors;
    FontConfig fonts;

    // 动画
    bool enableAnimations;
    int animationDuration;  // 毫秒

    // 圆角
    int borderRadius;

    // 阴影
    bool enableShadows;

    // 间距
    int compactSpacing;   // 紧凑间距
    int normalSpacing;    // 正常间距
    int relaxedSpacing;   // 宽松间距

    // 生成 QSS 样式表
    QString generateStyleSheet() const;
};

/**
 * @brief 无障碍主题管理器
 *
 * 功能：
 * - 管理多种主题模式
 * - WCAG 对比度验证
 * - 动态切换主题
 * - 用户自定义主题
 */
class AccessibilityThemeManager : public QObject {
    Q_OBJECT

public:
    static AccessibilityThemeManager* instance();

    // 获取当前主题
    ThemeConfig currentTheme() const { return currentTheme_; }
    ThemeMode currentMode() const { return currentTheme_.mode; }

    // 切换主题
    void setTheme(ThemeMode mode);
    void setCustomTheme(const ThemeConfig &config);

    // 字体大小
    void setFontSizePreset(FontSizePreset preset);
    void increaseFontSize();
    void decreaseFontSize();
    void resetFontSize();

    // 对比度
    void setContrastLevel(ContrastLevel level);
    ContrastLevel currentContrastLevel() const;

    // 预设主题
    ThemeConfig getStandardTheme() const;
    ThemeConfig getHighContrastTheme() const;
    ThemeConfig getExtraHighContrastTheme() const;
    ThemeConfig getDarkTheme() const;
    ThemeConfig getDarkHighContrastTheme() const;

    // 验证对比度
    bool validateContrast(const QColor &fg, const QColor &bg, ContrastLevel level) const;
    double calculateContrast(const QColor &fg, const QColor &bg) const;

    // 导出/导入
    void saveTheme(const QString &filePath);
    void loadTheme(const QString &filePath);

    // 应用到应用程序
    void applyTheme();
    QString generateGlobalStyleSheet() const;

signals:
    void themeChanged(ThemeMode mode);
    void fontSizeChanged(FontSizePreset preset);
    void contrastLevelChanged(ContrastLevel level);

private:
    explicit AccessibilityThemeManager(QObject *parent = nullptr);
    ~AccessibilityThemeManager() override;

    void initializeThemes();
    double relativeLuminance(const QColor &color) const;

    static AccessibilityThemeManager *instance_;

    ThemeConfig currentTheme_;
    QHash<ThemeMode, ThemeConfig> themes_;
    FontSizePreset currentFontPreset_;
};

/**
 * @brief 主题预览小部件
 *
 * 用于设置界面中预览主题效果
 */
class ThemePreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit ThemePreviewWidget(QWidget *parent = nullptr);

    void setTheme(const ThemeConfig &config);
    ThemeConfig theme() const { return theme_; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    ThemeConfig theme_;
};

} // namespace IntelNet

#endif // ACCESSIBILITY_THEME_H
