// 语音助手面板
#ifndef VOICE_PANEL_H
#define VOICE_PANEL_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QString>
#include <QPoint>
#include <QColor>

#include "rust_bridge.h"

namespace IntelNet {

class VoicePanel : public QWidget {
    Q_OBJECT

public:
    explicit VoicePanel(QWidget *parent = nullptr);
    ~VoicePanel();

    void setRustBridge(RustBridge *bridge);
    void analyzeImageFromFile(const QString &filePath);
    void analyzeImageData(const QString &imageData);

    // 将面板停靠到锚点窗口（主窗口）的右下角，并限制在屏幕内
    void adjustPosition(QWidget *anchor);

    // 供主窗口调用的状态展示接口
    void showBusy(const QString &message);
    void showTextResult(const QString &text);
    void showError(const QString &message);

signals:
    void analyzePageRequested();
    void analyzeImageRequested();
    void uploadImageRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onAnalyzePageClicked();
    void onAnalyzeImageClicked();
    void onUploadImageClicked();
    void onCloseClicked();

private:
    void setupUi();
    QWidget* buildHeader();
    QWidget* buildStatusCard();
    QPushButton* createActionButton(const QString &icon, const QString &title,
                                    const QString &hint, QWidget *parent);
    void updateStatus(const QString &status, const QColor &dotColor);
    void setVoiceText(const QString &text);
    void onImageAnalysisComplete(const QString &result, bool success);
    void enablePlayback(bool enabled);
    bool ensureModelInitialized();
    void startImageAnalysis(const QString &imageData);
    void setPlaying(bool playing);

    // UI 组件
    QLabel *statusLabel_;
    QLabel *statusDot_;
    QTextEdit *currentTextEdit_;
    QPushButton *playPauseBtn_;
    QPushButton *stopBtn_;
    QPushButton *analyzePageBtn_;
    QPushButton *analyzeImageBtn_;
    QPushButton *uploadImageBtn_;
    QPushButton *closeBtn_;

    // Rust 桥接
    RustBridge *rustBridge_;

    // 状态
    bool isPlaying_;
    bool modelInitialized_;

    // 拖动
    bool dragging_;
    QPoint dragStartPos_;
};

} // namespace IntelNet

#endif // VOICE_PANEL_H
