// 简化版无障碍工具栏实现
#include "simple_accessible_toolbar.h"
#include <QDebug>

namespace IntelNet {

SimpleAccessibleToolbar::SimpleAccessibleToolbar(QWidget *parent)
    : QWidget(parent)
    , currentParagraph_(0)
    , totalParagraphs_(0)
    , currentSpeed_(1.0)
    , isPlaying_(false)
{
    setupUi();
}

void SimpleAccessibleToolbar::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // ===== 顶部行 =====
    auto *topLayout = new QHBoxLayout();
    topLayout->setSpacing(12);

    // 语音按钮
    voiceButton_ = new QPushButton("🎤 语音输入", this);
    voiceButton_->setMinimumSize(120, 56);
    voiceButton_->setStyleSheet(R"(
        QPushButton {
            background: #0066FF;
            color: white;
            border: none;
            border-radius: 28px;
            font-size: 14pt;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #0052CC;
        }
        QPushButton:pressed {
            background: #003D99;
        }
    )");

    // 标题
    titleLabel_ = new QLabel("加载中...", this);
    titleLabel_->setStyleSheet(R"(
        QLabel {
            font-size: 16pt;
            font-weight: bold;
            color: #333;
        }
    )");

    // AI 助手按钮
    aiButton_ = new QPushButton("AI 助手", this);
    aiButton_->setMinimumSize(100, 44);
    aiButton_->setStyleSheet(R"(
        QPushButton {
            background: white;
            border: 2px solid #0066FF;
            border-radius: 8px;
            color: #0066FF;
            font-size: 12pt;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #F0F7FF;
        }
    )");

    topLayout->addWidget(voiceButton_);
    topLayout->addWidget(titleLabel_, 1);
    topLayout->addWidget(aiButton_);

    mainLayout->addLayout(topLayout);

    // ===== 分隔线 =====
    auto *separator1 = new QFrame(this);
    separator1->setFrameShape(QFrame::HLine);
    separator1->setStyleSheet("background: #E0E0E0;");
    separator1->setFixedHeight(1);
    mainLayout->addWidget(separator1);

    // ===== 朗读控制条 =====
    readingWidget_ = new QWidget(this);
    auto *readingLayout = new QVBoxLayout(readingWidget_);
    readingLayout->setContentsMargins(0, 8, 0, 8);
    readingLayout->setSpacing(12);

    // 进度条
    auto *progressLayout = new QHBoxLayout();
    progressBar_ = new QProgressBar(this);
    progressBar_->setTextVisible(false);
    progressBar_->setFixedHeight(8);
    progressBar_->setStyleSheet(R"(
        QProgressBar {
            border: none;
            background: #E0E0E0;
            border-radius: 4px;
        }
        QProgressBar::chunk {
            background: #0066FF;
            border-radius: 4px;
        }
    )");

    progressLabel_ = new QLabel("第 0 段 / 共 0 段", this);
    progressLabel_->setMinimumWidth(150);
    progressLabel_->setStyleSheet("font-size: 10pt; color: #666;");

    progressLayout->addWidget(progressBar_, 1);
    progressLayout->addWidget(progressLabel_);
    readingLayout->addLayout(progressLayout);

    // 控制按钮
    auto *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(12);

    QString buttonStyle = R"(
        QPushButton {
            background: white;
            border: 1px solid #D0D0D0;
            border-radius: 8px;
            padding: 8px 16px;
            font-size: 11pt;
        }
        QPushButton:hover {
            background: #F5F5F5;
            border-color: #0066FF;
        }
        QPushButton:pressed {
            background: #E8E8E8;
        }
    )";

    prevButton_ = new QPushButton("⏮ 上一段", this);
    prevButton_->setMinimumSize(100, 44);
    prevButton_->setStyleSheet(buttonStyle);

    playPauseButton_ = new QPushButton("▶ 播放", this);
    playPauseButton_->setMinimumSize(100, 44);
    playPauseButton_->setStyleSheet(buttonStyle);

    nextButton_ = new QPushButton("⏭ 下一段", this);
    nextButton_->setMinimumSize(100, 44);
    nextButton_->setStyleSheet(buttonStyle);

    speedButton_ = new QPushButton("速度: 1.0x", this);
    speedButton_->setMinimumSize(100, 44);
    speedButton_->setStyleSheet(buttonStyle);

    controlLayout->addWidget(prevButton_);
    controlLayout->addWidget(playPauseButton_);
    controlLayout->addWidget(nextButton_);
    controlLayout->addStretch();
    controlLayout->addWidget(speedButton_);

    readingLayout->addLayout(controlLayout);
    mainLayout->addWidget(readingWidget_);
    readingWidget_->hide(); // 初始隐藏

    // ===== 分隔线 =====
    auto *separator2 = new QFrame(this);
    separator2->setFrameShape(QFrame::HLine);
    separator2->setStyleSheet("background: #E0E0E0;");
    separator2->setFixedHeight(1);
    mainLayout->addWidget(separator2);

    // ===== 快捷操作栏 =====
    auto *quickLayout = new QHBoxLayout();
    quickLayout->setSpacing(12);

    readingModeButton_ = new QPushButton("📖 阅读模式", this);
    readingModeButton_->setMinimumSize(120, 44);
    readingModeButton_->setStyleSheet(buttonStyle);

    summarizeButton_ = new QPushButton("📝 总结", this);
    summarizeButton_->setMinimumSize(100, 44);
    summarizeButton_->setStyleSheet(buttonStyle);

    translateButton_ = new QPushButton("🌐 翻译", this);
    translateButton_->setMinimumSize(100, 44);
    translateButton_->setStyleSheet(buttonStyle);

    bookmarkButton_ = new QPushButton("⭐ 收藏", this);
    bookmarkButton_->setMinimumSize(100, 44);
    bookmarkButton_->setStyleSheet(buttonStyle);

    quickLayout->addWidget(readingModeButton_);
    quickLayout->addWidget(summarizeButton_);
    quickLayout->addWidget(translateButton_);
    quickLayout->addWidget(bookmarkButton_);
    quickLayout->addStretch();

    mainLayout->addLayout(quickLayout);

    // 设置整体样式
    setStyleSheet("background: white;");

    // 连接信号
    connect(voiceButton_, &QPushButton::clicked, this, &SimpleAccessibleToolbar::voiceClicked);
    connect(aiButton_, &QPushButton::clicked, this, &SimpleAccessibleToolbar::aiAssistantClicked);
    connect(prevButton_, &QPushButton::clicked, this, &SimpleAccessibleToolbar::previousParagraphClicked);
    connect(playPauseButton_, &QPushButton::clicked, this, &SimpleAccessibleToolbar::playPauseClicked);
    connect(nextButton_, &QPushButton::clicked, this, &SimpleAccessibleToolbar::nextParagraphClicked);
    connect(readingModeButton_, &QPushButton::clicked, this, &SimpleAccessibleToolbar::readingModeClicked);
    connect(summarizeButton_, &QPushButton::clicked, this, &SimpleAccessibleToolbar::summarizeClicked);
    connect(translateButton_, &QPushButton::clicked, this, &SimpleAccessibleToolbar::translateClicked);
    connect(bookmarkButton_, &QPushButton::clicked, this, &SimpleAccessibleToolbar::bookmarkClicked);

    connect(speedButton_, &QPushButton::clicked, this, [this]() {
        qreal speeds[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0};
        int currentIndex = 2; // 默认 1.0x
        for (int i = 0; i < 6; ++i) {
            if (qAbs(currentSpeed_ - speeds[i]) < 0.01) {
                currentIndex = i;
                break;
            }
        }
        currentSpeed_ = speeds[(currentIndex + 1) % 6];
        setReadingSpeed(currentSpeed_);
    });
}

void SimpleAccessibleToolbar::setPageInfo(const QString &title, const QString &source, int readingMinutes)
{
    QString fullTitle = title;
    if (!source.isEmpty()) {
        fullTitle += QString(" • %1").arg(source);
    }
    if (readingMinutes > 0) {
        fullTitle += QString(" • 约 %1 分钟").arg(readingMinutes);
    }
    titleLabel_->setText(fullTitle);
}

void SimpleAccessibleToolbar::setReadingProgress(int current, int total)
{
    currentParagraph_ = current;
    totalParagraphs_ = total;

    if (total > 0) {
        progressBar_->setMaximum(total);
        progressBar_->setValue(current);
        progressLabel_->setText(QString("第 %1 段 / 共 %2 段").arg(current).arg(total));
    }
}

void SimpleAccessibleToolbar::setReadingSpeed(qreal speed)
{
    currentSpeed_ = speed;
    speedButton_->setText(QString("速度: %1x").arg(speed, 0, 'f', 2));
}

void SimpleAccessibleToolbar::setReadingState(bool playing)
{
    isPlaying_ = playing;
    playPauseButton_->setText(playing ? "⏸ 暂停" : "▶ 播放");
}

void SimpleAccessibleToolbar::setAIUnreadCount(int count)
{
    if (count > 0) {
        aiButton_->setText(QString("AI 助手 (%1)").arg(count));
    } else {
        aiButton_->setText("AI 助手");
    }
}

void SimpleAccessibleToolbar::showReadingControls()
{
    readingWidget_->show();
}

void SimpleAccessibleToolbar::hideReadingControls()
{
    readingWidget_->hide();
}

} // namespace IntelNet
