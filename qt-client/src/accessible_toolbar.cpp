// 无障碍顶部工具栏实现 - IntelNet 浏览器
#include "accessible_toolbar.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QTimer>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QToolTip>
#include <QAccessible>
#include <QKeyEvent>
#include <QStyle>
#include <QStyleOption>
#include <cmath>

namespace IntelNet {

// ============================================================================
// VoiceButton 实现
// ============================================================================

VoiceButton::VoiceButton(QWidget *parent)
    : QPushButton(parent)
    , state_(Idle)
    , pulseOpacity_(1.0)
    , isHovered_(false)
    , pulseAnimation_(nullptr)
{
    setFixedSize(56, 56);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);

    // 设置无障碍属性
    setAccessibleName(tr("语音输入"));
    setAccessibleDescription(tr("按住或点击开始语音输入，说"帮助"查看命令列表"));

    // 设置工具提示
    setToolTip(tr("语音输入\n按住: 短命令\n点击: 长对话\n快捷键: Ctrl+Space"));

    // 创建脉动动画
    pulseAnimation_ = new QPropertyAnimation(this, "pulseOpacity", this);
    pulseAnimation_->setDuration(1000);
    pulseAnimation_->setLoopCount(-1);
    pulseAnimation_->setEasingCurve(QEasingCurve::InOutSine);
}

void VoiceButton::setState(State state)
{
    if (state_ == state) return;

    state_ = state;

    switch (state_) {
    case Idle:
        stopPulseAnimation();
        setAccessibleDescription(tr("按住或点击开始语音输入"));
        break;

    case Listening:
        startPulseAnimation();
        pulseAnimation_->setStartValue(0.5);
        pulseAnimation_->setEndValue(1.0);
        setAccessibleDescription(tr("正在聆听，请说话"));
        // 通知屏幕阅读器状态变化
        QAccessibleEvent event(this, QAccessible::StateChanged);
        QAccessible::updateAccessibility(&event);
        break;

    case Processing:
        startPulseAnimation();
        pulseAnimation_->setStartValue(0.3);
        pulseAnimation_->setEndValue(0.7);
        setAccessibleDescription(tr("正在识别，请稍候"));
        QAccessibleEvent event2(this, QAccessible::StateChanged);
        QAccessible::updateAccessibility(&event2);
        break;
    }

    update();
}

void VoiceButton::setPulseOpacity(qreal opacity)
{
    pulseOpacity_ = opacity;
    update();
}

void VoiceButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF rect = this->rect().adjusted(4, 4, -4, -4);

    // 根据状态选择颜色
    QColor baseColor;
    switch (state_) {
    case Idle:
        baseColor = QColor(0, 102, 255); // #0066FF
        break;
    case Listening:
        baseColor = QColor(255, 59, 48); // 红色
        break;
    case Processing:
        baseColor = QColor(255, 204, 0); // 黄色
        break;
    }

    // 悬停效果
    if (isHovered_ && state_ == Idle) {
        rect = rect.adjusted(-2, -2, 2, 2); // 放大 1.05x
    }

    // 绘制外圈（脉动效果）
    if (state_ != Idle) {
        painter.setOpacity(pulseOpacity_ * 0.3);
        painter.setBrush(baseColor);
        painter.setPen(Qt::NoPen);
        QRectF outerRect = rect.adjusted(-8, -8, 8, 8);
        painter.drawEllipse(outerRect);
    }

    // 绘制主按钮
    painter.setOpacity(state_ == Idle ? 0.5 : 1.0);
    if (hasFocus()) {
        // 焦点指示器
        painter.setPen(QPen(baseColor, 3));
        painter.setBrush(Qt::NoBrush);
        QRectF focusRect = rect.adjusted(-2, -2, 2, 2);
        painter.drawEllipse(focusRect);
    }

    painter.setOpacity(1.0);
    painter.setBrush(baseColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(rect);

    // 绘制麦克风图标
    painter.setPen(QPen(Qt::white, 2));
    QRectF micRect = rect.adjusted(16, 12, -16, -12);

    // 麦克风主体
    painter.drawRoundedRect(micRect.adjusted(0, 0, 0, -8), 4, 4);

    // 麦克风底座
    QPainterPath path;
    path.moveTo(micRect.center().x(), micRect.bottom() - 4);
    path.lineTo(micRect.center().x(), micRect.bottom() + 4);
    path.moveTo(micRect.left() - 2, micRect.bottom() + 4);
    path.lineTo(micRect.right() + 2, micRect.bottom() + 4);
    painter.drawPath(path);

    // 绘制文字（状态）
    if (state_ != Idle) {
        QFont font = painter.font();
        font.setPointSize(8);
        painter.setFont(font);
        painter.setPen(Qt::white);

        QString stateText = state_ == Listening ? tr("聆听中") : tr("识别中");
        QRectF textRect(0, rect.bottom() + 8, width(), 20);
        painter.drawText(textRect, Qt::AlignCenter, stateText);
    }
}

void VoiceButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit pressAndHold();
    }
    QPushButton::mousePressEvent(event);
}

void VoiceButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 判断是短按还是长按
        // 这里简化处理，实际可以用 QTimer 判断
        emit toggleMode();
    }
    QPushButton::mouseReleaseEvent(event);
}

void VoiceButton::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    isHovered_ = true;
    update();
}

void VoiceButton::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    isHovered_ = false;
    update();
}

void VoiceButton::startPulseAnimation()
{
    if (pulseAnimation_->state() != QAbstractAnimation::Running) {
        pulseAnimation_->start();
    }
}

void VoiceButton::stopPulseAnimation()
{
    pulseAnimation_->stop();
    pulseOpacity_ = 1.0;
}

// ============================================================================
// SmartStatusBar 实现
// ============================================================================

SmartStatusBar::SmartStatusBar(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setAccessibleName(tr("页面状态栏"));
}

void SmartStatusBar::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(4);

    // 标题
    titleLabel_ = new QLabel(this);
    QFont titleFont;
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel_->setFont(titleFont);
    titleLabel_->setWordWrap(false);
    titleLabel_->setTextFormat(Qt::PlainText);
    layout->addWidget(titleLabel_);

    // 元信息
    metaLabel_ = new QLabel(this);
    QFont metaFont;
    metaFont.setPointSize(9);
    metaLabel_->setFont(metaFont);
    metaLabel_->setStyleSheet("color: #666666;");
    layout->addWidget(metaLabel_);

    // 当前状态
    statusLabel_ = new QLabel(this);
    statusLabel_->setFont(metaFont);
    statusLabel_->setStyleSheet("color: #0066FF;");
    layout->addWidget(statusLabel_);
    statusLabel_->hide();
}

void SmartStatusBar::setPageTitle(const QString &title)
{
    titleLabel_->setText(title);
    titleLabel_->setAccessibleName(tr("页面标题: %1").arg(title));
}

void SmartStatusBar::setSource(const QString &source)
{
    QString meta = metaLabel_->text();
    if (meta.isEmpty()) {
        metaLabel_->setText(source);
    } else {
        metaLabel_->setText(source + " | " + meta);
    }
}

void SmartStatusBar::setReadingTime(int minutes)
{
    QString timeStr = tr("约 %1 分钟阅读").arg(minutes);
    QString current = metaLabel_->text();

    if (current.contains("|")) {
        metaLabel_->setText(current + " | " + timeStr);
    } else if (!current.isEmpty()) {
        metaLabel_->setText(current + " | " + timeStr);
    } else {
        metaLabel_->setText(timeStr);
    }
}

void SmartStatusBar::setCurrentStatus(const QString &status)
{
    if (status.isEmpty()) {
        statusLabel_->hide();
    } else {
        statusLabel_->setText(status);
        statusLabel_->show();

        // 通知屏幕阅读器
        setAccessibleDescription(status);
        QAccessibleEvent event(this, QAccessible::Alert);
        QAccessible::updateAccessibility(&event);
    }
}

// ============================================================================
// ReadingControlBar 实现
// ============================================================================

ReadingControlBar::ReadingControlBar(QWidget *parent)
    : QWidget(parent)
    , currentParagraph_(0)
    , totalParagraphs_(0)
    , currentSpeed_(1.0)
    , isPlaying_(false)
{
    setupUi();
    setAccessibleName(tr("朗读控制栏"));
}

void ReadingControlBar::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);

    // 进度条和位置
    auto *progressLayout = new QHBoxLayout();

    progressBar_ = new QProgressBar(this);
    progressBar_->setTextVisible(false);
    progressBar_->setFixedHeight(6);
    progressBar_->setStyleSheet(R"(
        QProgressBar {
            border: none;
            background: #E0E0E0;
            border-radius: 3px;
        }
        QProgressBar::chunk {
            background: #0066FF;
            border-radius: 3px;
        }
    )");

    positionLabel_ = new QLabel(this);
    positionLabel_->setMinimumWidth(120);
    positionLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QFont labelFont;
    labelFont.setPointSize(9);
    positionLabel_->setFont(labelFont);

    progressLayout->addWidget(progressBar_, 1);
    progressLayout->addWidget(positionLabel_);
    layout->addLayout(progressLayout);

    // 控制按钮
    auto *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(12);

    prevButton_ = new QPushButton(tr("⏮ 上一段"), this);
    prevButton_->setMinimumSize(100, 44);
    prevButton_->setAccessibleName(tr("上一段"));

    playPauseButton_ = new QPushButton(tr("⏸ 暂停"), this);
    playPauseButton_->setMinimumSize(100, 44);
    playPauseButton_->setAccessibleName(tr("暂停"));

    nextButton_ = new QPushButton(tr("⏭ 下一段"), this);
    nextButton_->setMinimumSize(100, 44);
    nextButton_->setAccessibleName(tr("下一段"));

    speedButton_ = new QPushButton(tr("速度: 1.0x"), this);
    speedButton_->setMinimumSize(100, 44);
    speedButton_->setAccessibleName(tr("调整朗读速度，当前 1.0 倍"));

    controlLayout->addWidget(prevButton_);
    controlLayout->addWidget(playPauseButton_);
    controlLayout->addWidget(nextButton_);
    controlLayout->addStretch();
    controlLayout->addWidget(speedButton_);

    layout->addLayout(controlLayout);

    // 连接信号
    connect(prevButton_, &QPushButton::clicked, this, &ReadingControlBar::previousParagraph);
    connect(playPauseButton_, &QPushButton::clicked, this, &ReadingControlBar::togglePlayPause);
    connect(nextButton_, &QPushButton::clicked, this, &ReadingControlBar::nextParagraph);
    connect(speedButton_, &QPushButton::clicked, this, [this]() {
        // 循环速度: 0.5x -> 0.75x -> 1.0x -> 1.25x -> 1.5x -> 2.0x
        qreal speeds[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0};
        int currentIndex = 0;
        for (int i = 0; i < 6; ++i) {
            if (qAbs(currentSpeed_ - speeds[i]) < 0.01) {
                currentIndex = i;
                break;
            }
        }
        qreal newSpeed = speeds[(currentIndex + 1) % 6];
        setSpeed(newSpeed);
        emit speedChanged(newSpeed);
    });

    // 进度条点击跳转
    progressBar_->installEventFilter(this);
}

void ReadingControlBar::setProgress(int current, int total)
{
    currentParagraph_ = current;
    totalParagraphs_ = total;

    if (total > 0) {
        progressBar_->setMaximum(total);
        progressBar_->setValue(current);
        positionLabel_->setText(tr("第 %1 段 / 共 %2 段").arg(current).arg(total));

        setAccessibleDescription(tr("朗读进度：第 %1 段，共 %2 段").arg(current).arg(total));
    }
}

void ReadingControlBar::setSpeed(qreal speed)
{
    currentSpeed_ = speed;
    speedButton_->setText(tr("速度: %1x").arg(speed, 0, 'f', 2));
    speedButton_->setAccessibleName(tr("调整朗读速度，当前 %1 倍").arg(speed, 0, 'f', 1));
}

void ReadingControlBar::setPlaying(bool playing)
{
    isPlaying_ = playing;
    playPauseButton_->setText(playing ? tr("⏸ 暂停") : tr("▶ 播放"));
    playPauseButton_->setAccessibleName(playing ? tr("暂停") : tr("播放"));
}

// ============================================================================
// AIAssistantButton 实现
// ============================================================================

AIAssistantButton::AIAssistantButton(QWidget *parent)
    : QPushButton(parent)
    , unreadCount_(0)
{
    setText(tr("AI 助手"));
    setMinimumSize(100, 44);
    setAccessibleName(tr("AI 助手"));
}

void AIAssistantButton::setUnreadCount(int count)
{
    unreadCount_ = count;
    if (count > 0) {
        setAccessibleDescription(tr("有 %1 条未读建议").arg(count));
    } else {
        setAccessibleDescription(tr("打开 AI 助手面板"));
    }
    update();
}

void AIAssistantButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);

    if (unreadCount_ > 0) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制红色徽章
        int badgeSize = 20;
        QRectF badgeRect(width() - badgeSize - 4, 4, badgeSize, badgeSize);

        painter.setBrush(QColor(255, 59, 48));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(badgeRect);

        // 绘制数字
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(9);
        font.setBold(true);
        painter.setFont(font);

        QString countText = unreadCount_ > 9 ? "9+" : QString::number(unreadCount_);
        painter.drawText(badgeRect, Qt::AlignCenter, countText);
    }
}

// ============================================================================
// QuickActionBar 实现
// ============================================================================

QuickActionBar::QuickActionBar(QWidget *parent)
    : QWidget(parent)
    , isExpanded_(false)
{
    setupUi();
    setupAnimation();
    setMaximumHeight(0); // 初始收起
}

void QuickActionBar::setupUi()
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(12);

    readingModeButton_ = new QPushButton(tr("📖 阅读模式"), this);
    readingModeButton_->setMinimumSize(120, 44);

    summarizeButton_ = new QPushButton(tr("📝 总结"), this);
    summarizeButton_->setMinimumSize(100, 44);

    translateButton_ = new QPushButton(tr("🌐 翻译"), this);
    translateButton_->setMinimumSize(100, 44);

    bookmarkButton_ = new QPushButton(tr("⭐ 收藏"), this);
    bookmarkButton_->setMinimumSize(100, 44);

    layout->addWidget(readingModeButton_);
    layout->addWidget(summarizeButton_);
    layout->addWidget(translateButton_);
    layout->addWidget(bookmarkButton_);
    layout->addStretch();

    // 连接信号
    connect(readingModeButton_, &QPushButton::clicked, this, &QuickActionBar::readingModeClicked);
    connect(summarizeButton_, &QPushButton::clicked, this, &QuickActionBar::summarizeClicked);
    connect(translateButton_, &QPushButton::clicked, this, &QuickActionBar::translateClicked);
    connect(bookmarkButton_, &QPushButton::clicked, this, &QuickActionBar::bookmarkClicked);

    // 设置无障碍名称
    readingModeButton_->setAccessibleName(tr("切换阅读模式"));
    summarizeButton_->setAccessibleName(tr("总结页面内容"));
    translateButton_->setAccessibleName(tr("翻译页面"));
    bookmarkButton_->setAccessibleName(tr("添加书签"));
}

void QuickActionBar::setupAnimation()
{
    opacityEffect_ = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacityEffect_);

    expandAnimation_ = new QPropertyAnimation(this, "maximumHeight", this);
    expandAnimation_->setDuration(200);
    expandAnimation_->setEasingCurve(QEasingCurve::OutCubic);

    auto *opacityAnimation = new QPropertyAnimation(opacityEffect_, "opacity", this);
    opacityAnimation->setDuration(200);

    auto *group = new QParallelAnimationGroup(this);
    group->addAnimation(expandAnimation_);
    group->addAnimation(opacityAnimation);
}

void QuickActionBar::expand()
{
    if (isExpanded_) return;

    isExpanded_ = true;
    expandAnimation_->setStartValue(0);
    expandAnimation_->setEndValue(60);

    auto *opacityAnimation = qobject_cast<QPropertyAnimation*>(
        expandAnimation_->parent()->findChild<QPropertyAnimation*>());
    if (opacityAnimation) {
        opacityAnimation->setStartValue(0.0);
        opacityAnimation->setEndValue(1.0);
    }

    expandAnimation_->start();
}

void QuickActionBar::collapse()
{
    if (!isExpanded_) return;

    isExpanded_ = false;
    expandAnimation_->setStartValue(60);
    expandAnimation_->setEndValue(0);

    auto *opacityAnimation = qobject_cast<QPropertyAnimation*>(
        expandAnimation_->parent()->findChild<QPropertyAnimation*>());
    if (opacityAnimation) {
        opacityAnimation->setStartValue(1.0);
        opacityAnimation->setEndValue(0.0);
    }

    expandAnimation_->start();
}

// ============================================================================
// AccessibleToolbar 实现
// ============================================================================

AccessibleToolbar::AccessibleToolbar(QWidget *parent)
    : QWidget(parent)
    , isExpanded_(false)
{
    setupUi();
    setupConnections();
    setupAccessibility();
    setupKeyboardNavigation();
    setupAutoCollapse();
}

void AccessibleToolbar::setupUi()
{
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(0, 0, 0, 0);
    mainLayout_->setSpacing(0);

    // 顶部行：语音按钮 + AI 助手 + 菜单
    topRowLayout_ = new QHBoxLayout();
    topRowLayout_->setContentsMargins(12, 12, 12, 12);
    topRowLayout_->setSpacing(12);

    voiceButton_ = new VoiceButton(this);
    aiButton_ = new AIAssistantButton(this);
    menuButton_ = new QPushButton(tr("更多 ≡"), this);
    menuButton_->setMinimumSize(80, 44);

    topRowLayout_->addWidget(voiceButton_);
    topRowLayout_->addStretch();
    topRowLayout_->addWidget(aiButton_);
    topRowLayout_->addWidget(menuButton_);

    mainLayout_->addLayout(topRowLayout_);

    // 分隔线
    auto *separator1 = new QFrame(this);
    separator1->setFrameShape(QFrame::HLine);
    separator1->setStyleSheet("background: #E0E0E0;");
    separator1->setFixedHeight(1);
    mainLayout_->addWidget(separator1);

    // 智能状态栏
    statusBar_ = new SmartStatusBar(this);
    mainLayout_->addWidget(statusBar_);

    // 朗读控制条（初始隐藏）
    readingBar_ = new ReadingControlBar(this);
    readingBar_->hide();
    mainLayout_->addWidget(readingBar_);

    // 分隔线
    auto *separator2 = new QFrame(this);
    separator2->setFrameShape(QFrame::HLine);
    separator2->setStyleSheet("background: #E0E0E0;");
    separator2->setFixedHeight(1);
    mainLayout_->addWidget(separator2);

    // 快捷操作栏
    quickActions_ = new QuickActionBar(this);
    mainLayout_->addWidget(quickActions_);

    // 设置样式
    setStyleSheet(R"(
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
        QPushButton:focus {
            border: 3px solid #0066FF;
            outline: none;
        }
        QPushButton:pressed {
            background: #E8E8E8;
        }
    )");
}

void AccessibleToolbar::setupConnections()
{
    // 语音按钮
    connect(voiceButton_, &VoiceButton::pressAndHold, this, &AccessibleToolbar::voicePressAndHold);
    connect(voiceButton_, &VoiceButton::toggleMode, this, &AccessibleToolbar::voiceToggleMode);

    // AI 助手
    connect(aiButton_, &QPushButton::clicked, this, &AccessibleToolbar::aiAssistantRequested);

    // 菜单
    connect(menuButton_, &QPushButton::clicked, this, &AccessibleToolbar::menuRequested);

    // 朗读控制
    connect(readingBar_, &ReadingControlBar::previousParagraph, this, &AccessibleToolbar::readingPrevious);
    connect(readingBar_, &ReadingControlBar::togglePlayPause, this, &AccessibleToolbar::readingTogglePlayPause);
    connect(readingBar_, &ReadingControlBar::nextParagraph, this, &AccessibleToolbar::readingNext);
    connect(readingBar_, &ReadingControlBar::speedChanged, this, &AccessibleToolbar::readingSpeedChanged);

    // 快捷操作
    connect(quickActions_, &QuickActionBar::readingModeClicked, this, &AccessibleToolbar::readingModeRequested);
    connect(quickActions_, &QuickActionBar::summarizeClicked, this, &AccessibleToolbar::summarizeRequested);
    connect(quickActions_, &QuickActionBar::translateClicked, this, &AccessibleToolbar::translateRequested);
    connect(quickActions_, &QuickActionBar::bookmarkClicked, this, &AccessibleToolbar::bookmarkRequested);
}

void AccessibleToolbar::setupAccessibility()
{
    setAccessibleName(tr("主工具栏"));
    setAccessibleDescription(tr("包含语音输入、页面信息和快捷操作"));

    // 设置焦点策略
    setFocusPolicy(Qt::StrongFocus);
}

void AccessibleToolbar::setupKeyboardNavigation()
{
    // 安装事件过滤器来处理键盘导航
    installEventFilter(this);
}

void AccessibleToolbar::setupAutoCollapse()
{
    collapseTimer_ = new QTimer(this);
    collapseTimer_->setInterval(5000); // 5秒后自动收起
    collapseTimer_->setSingleShot(true);

    connect(collapseTimer_, &QTimer::timeout, this, [this]() {
        if (isExpanded_) {
            collapseQuickActions();
        }
    });
}

void AccessibleToolbar::setPageInfo(const QString &title, const QString &source, int readingMinutes)
{
    statusBar_->setPageTitle(title);
    statusBar_->setSource(source);
    statusBar_->setReadingTime(readingMinutes);
}

void AccessibleToolbar::setReadingProgress(int current, int total)
{
    readingBar_->setProgress(current, total);
}

void AccessibleToolbar::setReadingSpeed(qreal speed)
{
    readingBar_->setSpeed(speed);
}

void AccessibleToolbar::setReadingState(bool playing)
{
    readingBar_->setPlaying(playing);
}

void AccessibleToolbar::setAIUnreadCount(int count)
{
    aiButton_->setUnreadCount(count);
}

void AccessibleToolbar::showReadingControls()
{
    readingBar_->show();
    statusBar_->setCurrentStatus(tr("● 朗读中..."));
}

void AccessibleToolbar::hideReadingControls()
{
    readingBar_->hide();
    statusBar_->setCurrentStatus("");
}

void AccessibleToolbar::expandQuickActions()
{
    quickActions_->expand();
    isExpanded_ = true;
    collapseTimer_->start();
}

void AccessibleToolbar::collapseQuickActions()
{
    quickActions_->collapse();
    isExpanded_ = false;
    collapseTimer_->stop();
}

void AccessibleToolbar::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    expandQuickActions();
}

void AccessibleToolbar::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    // 启动自动收起计时器
    if (isExpanded_) {
        collapseTimer_->start();
    }
}

bool AccessibleToolbar::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // Ctrl+Space: 激活语音输入
        if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_Space) {
            voiceButton_->click();
            return true;
        }

        // 展开快捷操作栏
        if (!isExpanded_ && (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Down)) {
            expandQuickActions();
        }
    }

    return QWidget::eventFilter(obj, event);
}

} // namespace IntelNet
