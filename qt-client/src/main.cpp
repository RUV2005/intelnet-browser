// 程序入口
#include <QApplication>
#include <QIcon>
#include <iostream>
#include "mainwindow.h"
#include "theme.h"

int main(int argc, char *argv[])
{
    // Qt 6 默认启用高 DPI 缩放，无需手动设置
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("IntelNet Browser");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("IntelNet");
    app.setOrganizationDomain("intelnet.com");

    // 应用全局主题（Chrome / Edge 风格）
    IntelNet::Theme::applyToApplication(app);

    std::cout << "IntelNet 无障碍浏览器启动中..." << std::endl;

    // 创建并显示主窗口
    IntelNet::MainWindow mainWindow;
    mainWindow.show();

    std::cout << "✓ IntelNet 浏览器已启动" << std::endl;

    return app.exec();
}
