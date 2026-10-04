// 主窗口 - IntelNet 浏览器
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QAction>
#include <QMenu>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QList>
#include <memory>

#include "browser_widget.h"
#include "voice_panel.h"
#include "rust_bridge.h"

namespace IntelNet {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private slots:
    // 浏览器导航
    void onGoButtonClicked();
    void onBackButtonClicked();
    void onForwardButtonClicked();
    void onRefreshButtonClicked();
    void onUrlChanged(const QUrl &url);
    void onLoadFinished(bool ok);
    void onPopupCloseButtonDetected();
    void onRedirectChainBlocked(const QUrl &url);
    void onWarningActionRequested(const QString &action, const QUrl &url);
    void onRepeatedAlertBlocked();

    // 语音助手
    void onVoiceButtonClicked();
    void onAnalyzePageClicked();
    void onAnalyzeImageClicked();
    void onDescribeMissingImagesClicked();
    void onUploadImageClicked();

private:
    void setupUi();
    void setupToolbar();
    void setupConnections();
    void initializeRustCore();
    void showVoicePanel();
    void showRedirectWarning(const QUrl &url);
    void analyzeNextMissingImage();

    // UI 组件
    QToolBar *toolbar_;
    QLineEdit *urlInput_;
    QPushButton *goButton_;
    QAction *backAction_;
    QAction *forwardAction_;
    QAction *refreshAction_;
    QPushButton *voiceButton_;
    QToolButton *menuButton_;

    // 核心组件
    BrowserWidget *browserWidget_;
    VoicePanel *voicePanel_;

    // Rust 桥接
    std::unique_ptr<RustBridge> rustBridge_;

    // 当前页面的无 alt 图片分析队列
    QList<BrowserWidget::MissingAltImage> missingAltImages_;
    int missingAltIndex_;
    bool analyzingMissingImages_;
};

} // namespace IntelNet

#endif // MAINWINDOW_H
