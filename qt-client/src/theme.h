// 主题工具：统一调色板、字体、全局样式表、阴影
#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QPoint>
#include <QString>

class QApplication;
class QWidget;
class QGraphicsDropShadowEffect;

namespace IntelNet {
namespace Theme {

// 调色板
namespace Color {
    extern const QColor Primary;        // #0B57D0
    extern const QColor PrimaryHover;   // #0A50B8
    extern const QColor PrimaryText;    // #0B57D0
    extern const QColor Tonal;          // #E8F0FE
    extern const QColor TonalHover;     // #D3E3FD

    extern const QColor TextPrimary;    // #1F1F1F
    extern const QColor TextSecondary;  // #5F6368
    extern const QColor TextDisabled;   // #8A919A
    extern const QColor IconDefault;    // #3C4043

    extern const QColor BgPrimary;      // #FFFFFF
    extern const QColor BgSecondary;    // #F8F9FA
    extern const QColor BgUrlBar;       // #F1F3F4
    extern const QColor BgHover;        // #E9E9EA

    extern const QColor Border;         // #DADCE0
    extern const QColor BorderLight;    // #E0E3E6

    extern const QColor Success;        // #137333
    extern const QColor Warning;        // #B06000
    extern const QColor Danger;         // #D93025
} // namespace Color

// 加载并应用全局样式表
void applyToApplication(QApplication &app);

// 给无边框浮动窗口加柔和投影
QGraphicsDropShadowEffect* addShadow(QWidget *widget,
                                     int radius = 24,
                                     const QColor &color = QColor(60, 64, 67, 90),
                                     const QPoint &offset = QPoint(0, 6));

// 图标路径
inline QString icon(const QString &name) {
    return QStringLiteral(":/resources/icons/") + name + QStringLiteral(".svg");
}

} // namespace Theme
} // namespace IntelNet

#endif // THEME_H
