// 主题工具实现
#include "theme.h"
#include <QApplication>
#include <QFile>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QPalette>
#include <QWidget>

namespace IntelNet {
namespace Theme {

namespace Color {
    const QColor Primary(0x0B, 0x57, 0xD0);
    const QColor PrimaryHover(0x0A, 0x50, 0xB8);
    const QColor PrimaryText(0x0B, 0x57, 0xD0);
    const QColor Tonal(0xE8, 0xF0, 0xFE);
    const QColor TonalHover(0xD3, 0xE3, 0xFD);

    const QColor TextPrimary(0x1F, 0x1F, 0x1F);
    const QColor TextSecondary(0x5F, 0x63, 0x68);
    const QColor TextDisabled(0x8A, 0x91, 0x9A);
    const QColor IconDefault(0x3C, 0x40, 0x43);

    const QColor BgPrimary(0xFF, 0xFF, 0xFF);
    const QColor BgSecondary(0xF8, 0xF9, 0xFA);
    const QColor BgUrlBar(0xF1, 0xF3, 0xF4);
    const QColor BgHover(0xE9, 0xE9, 0xEA);

    const QColor Border(0xDA, 0xDC, 0xE0);
    const QColor BorderLight(0xE0, 0xE3, 0xE6);

    const QColor Success(0x13, 0x73, 0x33);
    const QColor Warning(0xB0, 0x60, 0x00);
    const QColor Danger(0xD9, 0x30, 0x25);
} // namespace Color

void applyToApplication(QApplication &app) {
    // 全局字体
    QFont font(QStringLiteral("Segoe UI"), 10);
    font.setFamilies({QStringLiteral("Segoe UI"), QStringLiteral("Microsoft YaHei")});
    app.setFont(font);

    // 应用调色板（用于未走 QSS 的默认控件）
    QPalette palette = app.palette();
    palette.setColor(QPalette::Window, Color::BgPrimary);
    palette.setColor(QPalette::WindowText, Color::TextPrimary);
    palette.setColor(QPalette::Base, Color::BgPrimary);
    palette.setColor(QPalette::Text, Color::TextPrimary);
    palette.setColor(QPalette::PlaceholderText, Color::TextDisabled);
    palette.setColor(QPalette::Button, Color::BgPrimary);
    palette.setColor(QPalette::ButtonText, Color::TextPrimary);
    palette.setColor(QPalette::Highlight, QColor(0xB4, 0xD5, 0xFE));
    app.setPalette(palette);

    // 加载样式表
    QFile file(QStringLiteral(":/resources/theme.qss"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(file.readAll()));
    }
}

QGraphicsDropShadowEffect* addShadow(QWidget *widget,
                                     int radius,
                                     const QColor &color,
                                     const QPoint &offset) {
    auto *effect = new QGraphicsDropShadowEffect(widget);
    effect->setBlurRadius(radius);
    effect->setColor(color);
    effect->setOffset(offset);
    widget->setGraphicsEffect(effect);
    return effect;
}

} // namespace Theme
} // namespace IntelNet
