// 浏览器组件 - 基于 QWebEngineView
#ifndef BROWSER_WIDGET_H
#define BROWSER_WIDGET_H

#include <QWidget>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QUrl>
#include <QStringList>
#include <functional>

class QTimer;

namespace IntelNet {

class BrowserWidget : public QWebEngineView {
    Q_OBJECT

public:
    explicit BrowserWidget(QWidget *parent = nullptr);
    ~BrowserWidget();

    // 导航方法
    void load(const QUrl &url);
    void back();
    void forward();
    void reload();

    // 异步获取当前页面可见文本
    void requestPageText(std::function<void(const QString&)> callback);

    // 异步获取当前页面的结构化源码（URL/标题/描述/标题层级/正文/图片/链接）
    void requestPageSource(std::function<void(const QString&)> callback);

    // 异步获取页面中所有图片的 URL
    void requestImageUrls(std::function<void(const QStringList&)> callback);

    // 让用户在页面中点击一张图片，回调返回其 data URL（取消时返回空字符串）
    void pickImage(std::function<void(const QString&)> callback);

    // 取消正在进行的图片选择
    void cancelPickImage();

signals:
    void urlChanged(const QUrl &url);
    void loadFinished(bool ok);
    void loadProgress(int progress);

private:
    void setupPage();
    void injectPickScript();

    QTimer *pickTimer_;
    std::function<void(const QString&)> pickCallback_;
    int pickElapsedMs_;
};

} // namespace IntelNet

#endif // BROWSER_WIDGET_H
