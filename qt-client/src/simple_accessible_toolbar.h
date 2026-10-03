// 简化版无障碍工具栏 - 可快速编译运行
#ifndef SIMPLE_ACCESSIBLE_TOOLBAR_H
#define SIMPLE_ACCESSIBLE_TOOLBAR_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>

namespace IntelNet {

class SimpleAccessibleToolbar : public QWidget {
    Q_OBJECT

public:
    explicit SimpleAccessibleToolbar(QWidget *parent = nullptr);

    void setPageInfo(const QString &title, const QString &source, int readingMinutes);
    void setReadingProgress(int current, int total);
    void setReadingSpeed(qreal speed);
    void setReadingState(bool playing);
    void setAIUnreadCount(int count);

    void showReadingControls();
    void hideReadingControls();

signals:
    void voiceClicked();
    void aiAssistantClicked();
    void readingModeClicked();
    void summarizeClicked();
    void translateClicked();
    void bookmarkClicked();
    void playPauseClicked();
    void nextParagraphClicked();
    void previousParagraphClicked();

private:
    void setupUi();

    // 顶部行
    QPushButton *voiceButton_;
    QLabel *titleLabel_;
    QPushButton *aiButton_;

    // 朗读控制
    QWidget *readingWidget_;
    QProgressBar *progressBar_;
    QLabel *progressLabel_;
    QPushButton *prevButton_;
    QPushButton *playPauseButton_;
    QPushButton *nextButton_;
    QPushButton *speedButton_;

    // 快捷操作
    QPushButton *readingModeButton_;
    QPushButton *summarizeButton_;
    QPushButton *translateButton_;
    QPushButton *bookmarkButton_;

    int currentParagraph_;
    int totalParagraphs_;
    qreal currentSpeed_;
    bool isPlaying_;
};

} // namespace IntelNet

#endif // SIMPLE_ACCESSIBLE_TOOLBAR_H
