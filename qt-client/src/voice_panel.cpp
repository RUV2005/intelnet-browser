// 语音助手面板实现（Chrome / Edge 风格卡片）
#include "voice_panel.h"
#include "theme.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFrame>
#include <QScrollArea>
#include <QIcon>
#include <QPixmap>
#include <QMouseEvent>
#include <QAbstractButton>
#include <QScreen>
#include <QGuiApplication>

namespace IntelNet {

namespace Color = Theme::Color;

VoicePanel::VoicePanel(QWidget *parent)
    : QWidget(parent)
    , statusLabel_(nullptr)
    , statusDot_(nullptr)
    , currentTextEdit_(nullptr)
    , playPauseBtn_(nullptr)
    , stopBtn_(nullptr)
    , analyzePageBtn_(nullptr)
    , analyzeImageBtn_(nullptr)
    , uploadImageBtn_(nullptr)
    , closeBtn_(nullptr)
    , rustBridge_(nullptr)
    , isPlaying_(false)
    , modelInitialized_(false)
    , dragging_(false)
{
    setupUi();
    // 位置在显示时由 adjustPosition() 计算。
}

VoicePanel::~VoicePanel() {
}

void VoicePanel::setupUi() {
    setFixedSize(420, 620);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    // 外层留白给投影
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(0);

    // 卡片
    QFrame *card = new QFrame(this);
    card->setObjectName("VoicePanel");
    Theme::addShadow(card, 28, QColor(60, 64, 67, 70), QPoint(0, 8));
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    // 标题栏
    cardLayout->addWidget(buildHeader());

    // 分隔线
    QFrame *sep = new QFrame(card);
    sep->setObjectName("PanelSeparator");
    sep->setFixedHeight(1);
    cardLayout->addWidget(sep);

    // 内容（可滚动）
    QScrollArea *scroll = new QScrollArea(card);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *content = new QWidget();
    content->setObjectName("PanelContent");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(18, 16, 18, 18);
    contentLayout->setSpacing(16);

    contentLayout->addWidget(buildStatusCard());

    // 控制按钮
    QHBoxLayout *controls = new QHBoxLayout();
    controls->setSpacing(10);

    playPauseBtn_ = new QPushButton("播放", content);
    playPauseBtn_->setObjectName("PrimaryButton");
    playPauseBtn_->setIcon(QIcon(Theme::icon("play_white")));
    playPauseBtn_->setIconSize(QSize(16, 16));
    playPauseBtn_->setEnabled(false);
    playPauseBtn_->setCursor(Qt::PointingHandCursor);
    playPauseBtn_->setAccessibleName("播放或暂停朗读");
    controls->addWidget(playPauseBtn_, 1);

    stopBtn_ = new QPushButton("停止", content);
    stopBtn_->setObjectName("SecondaryButton");
    stopBtn_->setIcon(QIcon(Theme::icon("stop_dark")));
    stopBtn_->setIconSize(QSize(14, 14));
    stopBtn_->setEnabled(false);
    stopBtn_->setCursor(Qt::PointingHandCursor);
    stopBtn_->setAccessibleName("停止朗读");
    controls->addWidget(stopBtn_, 1);

    contentLayout->addLayout(controls);

    // 快速功能
    QLabel *quickLabel = new QLabel("快速功能", content);
    quickLabel->setObjectName("SectionTitle");
    contentLayout->addWidget(quickLabel);

    analyzePageBtn_ = createActionButton("doc", "页面摘要", "Ctrl+Shift+S", content);
    contentLayout->addWidget(analyzePageBtn_);

    analyzeImageBtn_ = createActionButton("image", "图片分析", "Ctrl+Shift+A", content);
    contentLayout->addWidget(analyzeImageBtn_);

    uploadImageBtn_ = createActionButton("folder", "上传图片", "Ctrl+Shift+U", content);
    contentLayout->addWidget(uploadImageBtn_);

    contentLayout->addStretch();

    scroll->setWidget(content);
    cardLayout->addWidget(scroll, 1);

    root->addWidget(card);

    // 连接信号
    connect(closeBtn_, &QPushButton::clicked, this, &VoicePanel::onCloseClicked);
    connect(playPauseBtn_, &QPushButton::clicked, this, &VoicePanel::onPlayPauseClicked);
    connect(stopBtn_, &QPushButton::clicked, this, &VoicePanel::onStopClicked);
    connect(analyzePageBtn_, &QPushButton::clicked, this, &VoicePanel::onAnalyzePageClicked);
    connect(analyzeImageBtn_, &QPushButton::clicked, this, &VoicePanel::onAnalyzeImageClicked);
    connect(uploadImageBtn_, &QPushButton::clicked, this, &VoicePanel::onUploadImageClicked);
}

QWidget* VoicePanel::buildHeader() {
    QWidget *header = new QWidget();
    header->setObjectName("PanelHeader");
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(18, 14, 12, 14);
    hl->setSpacing(10);

    QLabel *iconLabel = new QLabel(header);
    iconLabel->setPixmap(QIcon(Theme::icon("sparkle")).pixmap(20, 20));
    iconLabel->setFixedSize(24, 24);
    iconLabel->setAlignment(Qt::AlignCenter);
    hl->addWidget(iconLabel);

    QVBoxLayout *titles = new QVBoxLayout();
    titles->setSpacing(0);
    QLabel *title = new QLabel("语音助手", header);
    title->setObjectName("PanelTitle");
    QLabel *subtitle = new QLabel("IntelNet Assistant", header);
    subtitle->setObjectName("PanelSubtitle");
    titles->addWidget(title);
    titles->addWidget(subtitle);
    hl->addLayout(titles);
    hl->addStretch();

    closeBtn_ = new QPushButton(header);
    closeBtn_->setObjectName("IconButton");
    closeBtn_->setIcon(QIcon(Theme::icon("close")));
    closeBtn_->setIconSize(QSize(16, 16));
    closeBtn_->setCursor(Qt::PointingHandCursor);
    closeBtn_->setAccessibleName("关闭语音助手面板");
    hl->addWidget(closeBtn_);

    return header;
}

QWidget* VoicePanel::buildStatusCard() {
    QFrame *card = new QFrame();
    card->setObjectName("StatusCard");
    QVBoxLayout *cl = new QVBoxLayout(card);
    cl->setContentsMargins(14, 12, 14, 14);
    cl->setSpacing(10);

    QHBoxLayout *top = new QHBoxLayout();
    top->setSpacing(8);
    statusDot_ = new QLabel(card);
    statusDot_->setObjectName("StatusDot");
    statusDot_->setFixedSize(12, 12);
    statusLabel_ = new QLabel("就绪", card);
    statusLabel_->setObjectName("StatusLabel");
    top->addWidget(statusDot_);
    top->addWidget(statusLabel_);
    top->addStretch();
    cl->addLayout(top);

    currentTextEdit_ = new QTextEdit(card);
    currentTextEdit_->setObjectName("CurrentText");
    currentTextEdit_->setReadOnly(true);
    currentTextEdit_->setPlainText("点击「快速功能」开始，例如生成页面摘要");
    currentTextEdit_->setMinimumHeight(90);
    currentTextEdit_->setAccessibleName("当前朗读文本");
    currentTextEdit_->setAccessibleDescription("AI 分析或朗读的内容会显示在这里");
    cl->addWidget(currentTextEdit_);

    return card;
}

QPushButton* VoicePanel::createActionButton(const QString &iconName, const QString &title,
                                            const QString &hint, QWidget *parent) {
    QPushButton *btn = new QPushButton(parent);
    btn->setObjectName("ActionButton");
    btn->setCursor(Qt::PointingHandCursor);
    btn->setAccessibleName(title);
    btn->setAccessibleDescription(hint);

    QHBoxLayout *hl = new QHBoxLayout(btn);
    hl->setContentsMargins(6, 8, 6, 8);
    hl->setSpacing(12);

    QLabel *iconLabel = new QLabel(btn);
    iconLabel->setPixmap(QIcon(Theme::icon(iconName)).pixmap(20, 20));
    iconLabel->setFixedSize(24, 24);
    iconLabel->setAlignment(Qt::AlignCenter);
    hl->addWidget(iconLabel);

    QVBoxLayout *texts = new QVBoxLayout();
    texts->setSpacing(2);
    QLabel *titleLabel = new QLabel(title, btn);
    titleLabel->setObjectName("ActionTitle");
    QLabel *hintLabel = new QLabel(hint, btn);
    hintLabel->setObjectName("ActionHint");
    texts->addWidget(titleLabel);
    texts->addWidget(hintLabel);

    hl->addLayout(texts);
    hl->addStretch();

    return btn;
}

void VoicePanel::setRustBridge(RustBridge *bridge) {
    rustBridge_ = bridge;
}

void VoicePanel::updateStatus(const QString &status, const QColor &dotColor) {
    statusLabel_->setText(status);
    statusDot_->setStyleSheet(QString("background: %1;").arg(dotColor.name()));
}

void VoicePanel::setVoiceText(const QString &text) {
    currentTextEdit_->setPlainText(text);
}

void VoicePanel::setPlaying(bool playing) {
    isPlaying_ = playing;
    if (playing) {
        playPauseBtn_->setText("暂停");
        playPauseBtn_->setIcon(QIcon(Theme::icon("pause_white")));
        playPauseBtn_->setAccessibleName("暂停朗读");
    } else {
        playPauseBtn_->setText("播放");
        playPauseBtn_->setIcon(QIcon(Theme::icon("play_white")));
        playPauseBtn_->setAccessibleName("播放朗读");
    }
}

void VoicePanel::adjustPosition(QWidget *anchor) {
    QScreen *screen = anchor ? anchor->screen() : QGuiApplication::primaryScreen();
    const QRect avail = screen ? screen->availableGeometry()
                               : QRect(0, 0, 1280, 800);

    const int margin = 24;
    const int maxH = avail.height() - margin * 2;
    if (maxH > 0 && height() > maxH) {
        setFixedSize(width(), maxH);
    }

    int x;
    int y;
    if (anchor) {
        const QRect g = anchor->frameGeometry();
        x = g.right() - width() - margin;
        y = g.bottom() - height() - margin;
    } else {
        x = avail.right() - width() - margin;
        y = avail.bottom() - height() - margin;
    }

    x = qBound(avail.left(), x, avail.right() - width());
    y = qBound(avail.top(), y, avail.bottom() - height());

    move(x, y);
}

bool VoicePanel::ensureModelInitialized() {
    if (!rustBridge_) {
        QMessageBox::warning(this, "错误", "Rust 核心未初始化");
        return false;
    }

    if (modelInitialized_) {
        return true;
    }

    updateStatus("正在初始化 AI 模型...", Color::Warning);
    if (!rustBridge_->InitModel()) {
        updateStatus("模型初始化失败", Color::Danger);
        QMessageBox::critical(this, "错误", "AI 模型初始化失败");
        return false;
    }
    modelInitialized_ = true;
    return true;
}

void VoicePanel::enablePlayback(bool enabled) {
    playPauseBtn_->setEnabled(enabled);
    stopBtn_->setEnabled(enabled);
}

void VoicePanel::startImageAnalysis(const QString &imageData) {
    if (imageData.isEmpty()) {
        return;
    }
    if (!ensureModelInitialized()) {
        return;
    }

    updateStatus("正在分析图片...", Color::Warning);
    setVoiceText("AI 正在分析图片内容...");
    enablePlayback(false);

    rustBridge_->AnalyzeImageAsync(imageData.toStdString(),
        [this](const std::string &result, bool success) {
            QMetaObject::invokeMethod(this, [this, result, success]() {
                onImageAnalysisComplete(QString::fromStdString(result), success);
            }, Qt::QueuedConnection);
        });
}

void VoicePanel::analyzeImageFromFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法读取图片文件");
        return;
    }

    const QByteArray data = file.readAll();
    const QString base64 = QString("data:image/png;base64,") + data.toBase64();
    startImageAnalysis(base64);
}

void VoicePanel::analyzeImageData(const QString &imageData) {
    startImageAnalysis(imageData);
}

void VoicePanel::showBusy(const QString &message) {
    updateStatus(message, Color::Warning);
    setVoiceText(message);
    enablePlayback(false);
}

void VoicePanel::showTextResult(const QString &text, bool speak) {
    updateStatus("分析完成", Color::Success);
    setVoiceText(text);
    enablePlayback(true);

    if (speak && rustBridge_ && !text.isEmpty()) {
        rustBridge_->Speak(text.toStdString());
        setPlaying(true);
        updateStatus("正在朗读", Color::Success);
    }
}

void VoicePanel::showError(const QString &message) {
    updateStatus("失败", Color::Danger);
    setVoiceText(message);
    enablePlayback(false);
}

void VoicePanel::onImageAnalysisComplete(const QString &result, bool success) {
    if (!success) {
        showError("图片分析失败，请重试");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
    QString text = doc.object().value("result").toString();

    if (text.isEmpty()) {
        showError("图片分析失败，请重试");
        return;
    }

    showTextResult(text, false);
}

// ===== 槽函数 =====

void VoicePanel::onPlayPauseClicked() {
    if (isPlaying_) {
        if (rustBridge_) {
            rustBridge_->StopSpeaking();
        }
        setPlaying(false);
        updateStatus("已暂停", Color::TextSecondary);
    } else {
        QString text = currentTextEdit_->toPlainText();
        if (rustBridge_ && !text.isEmpty()) {
            rustBridge_->Speak(text.toStdString());
        }
        setPlaying(true);
        updateStatus("正在朗读", Color::Success);
    }
}

void VoicePanel::onStopClicked() {
    if (rustBridge_) {
        rustBridge_->StopSpeaking();
    }
    setPlaying(false);
    updateStatus("已停止", Color::TextSecondary);
}

void VoicePanel::onAnalyzePageClicked() {
    emit analyzePageRequested();
}

void VoicePanel::onAnalyzeImageClicked() {
    emit analyzeImageRequested();
}

void VoicePanel::onUploadImageClicked() {
    emit uploadImageRequested();
}

void VoicePanel::onCloseClicked() {
    hide();
}

// ===== 拖动无边框窗口 =====

void VoicePanel::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        const QPoint pos = event->position().toPoint();
        QWidget *child = childAt(pos);
        // 仅在标题栏空白区域按下时开始拖动
        if (pos.y() < 64 && !qobject_cast<QAbstractButton*>(child)) {
            dragging_ = true;
            dragStartPos_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
        }
    }
    QWidget::mousePressEvent(event);
}

void VoicePanel::mouseMoveEvent(QMouseEvent *event) {
    if (dragging_) {
        move(event->globalPosition().toPoint() - dragStartPos_);
    }
    QWidget::mouseMoveEvent(event);
}

void VoicePanel::mouseReleaseEvent(QMouseEvent *event) {
    dragging_ = false;
    QWidget::mouseReleaseEvent(event);
}

} // namespace IntelNet
