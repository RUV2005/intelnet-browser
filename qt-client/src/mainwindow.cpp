// 主窗口实现
#include "mainwindow.h"
#include "theme.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QShortcut>
#include <QStatusBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QDebug>
#include <QTimer>

namespace IntelNet {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , toolbar_(nullptr)
    , urlInput_(nullptr)
    , goButton_(nullptr)
    , backAction_(nullptr)
    , forwardAction_(nullptr)
    , refreshAction_(nullptr)
    , voiceButton_(nullptr)
    , menuButton_(nullptr)
    , browserWidget_(nullptr)
    , voicePanel_(nullptr)
    , rustBridge_(nullptr)
    , missingAltIndex_(0)
    , analyzingMissingImages_(false)
{
    setupUi();
    setupToolbar();
    setupConnections();
    initializeRustCore();

    // 设置窗口属性
    setWindowTitle("IntelNet 无障碍浏览器");
    resize(1200, 800);
    statusBar()->showMessage("就绪");
}

MainWindow::~MainWindow() {
    if (rustBridge_) {
        rustBridge_->Shutdown();
    }
}

void MainWindow::setupUi() {
    // 创建中心部件容器
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建浏览器组件
    browserWidget_ = new BrowserWidget(this);
    mainLayout->addWidget(browserWidget_);

    // 创建语音助手面板（浮动窗口）
    voicePanel_ = new VoicePanel(this);
    voicePanel_->hide();  // 默认隐藏

    setCentralWidget(centralWidget);
}

void MainWindow::setupToolbar() {
    using Theme::icon;

    // 创建工具栏（Chrome 风格：白底 + 1px 分隔线）
    toolbar_ = addToolBar("导航栏");
    toolbar_->setObjectName("NavToolBar");
    toolbar_->setMovable(false);
    toolbar_->setFloatable(false);
    toolbar_->setIconSize(QSize(18, 18));
    toolbar_->setToolButtonStyle(Qt::ToolButtonIconOnly);

    // 后退
    backAction_ = toolbar_->addAction(QIcon(icon("back")), "后退");
    backAction_->setShortcut(QKeySequence("Alt+Left"));

    // 前进
    forwardAction_ = toolbar_->addAction(QIcon(icon("forward")), "前进");
    forwardAction_->setShortcut(QKeySequence("Alt+Right"));

    // 刷新
    refreshAction_ = toolbar_->addAction(QIcon(icon("reload")), "刷新");
    refreshAction_->setShortcut(QKeySequence("F5"));

    toolbar_->addSeparator();

    // 地址栏（Omnibox）
    urlInput_ = new QLineEdit(toolbar_);
    urlInput_->setObjectName("UrlInput");
    urlInput_->setPlaceholderText("搜索或输入网址");
    urlInput_->setClearButtonEnabled(true);
    urlInput_->setAccessibleName("网址输入框");
    urlInput_->setAccessibleDescription("输入网址或搜索关键词，按回车前往");
    urlInput_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    urlInput_->setMinimumWidth(280);
    toolbar_->addWidget(urlInput_); // 由 Expanding size policy 拉伸

    // Go 按钮
    goButton_ = new QPushButton("前往", toolbar_);
    goButton_->setObjectName("GoButton");
    goButton_->setCursor(Qt::PointingHandCursor);
    goButton_->setAccessibleName("前往");
    toolbar_->addWidget(goButton_);

    // 语音助手按钮（filled tonal）
    voiceButton_ = new QPushButton(QIcon(icon("mic")), "语音助手", toolbar_);
    voiceButton_->setObjectName("VoiceButton");
    voiceButton_->setCursor(Qt::PointingHandCursor);
    voiceButton_->setAccessibleName("打开语音助手面板");
    voiceButton_->setAccessibleDescription("打开或关闭语音助手面板");
    toolbar_->addWidget(voiceButton_);

    // 菜单按钮（三点）
    menuButton_ = new QToolButton(toolbar_);
    menuButton_->setObjectName("MenuButton");
    menuButton_->setIcon(QIcon(icon("menu")));
    menuButton_->setPopupMode(QToolButton::InstantPopup);
    menuButton_->setCursor(Qt::PointingHandCursor);
    menuButton_->setAccessibleName("菜单");
    menuButton_->setAccessibleDescription("打开设置和更多选项");

    QMenu *menu = new QMenu(menuButton_);
    QAction *shortcutsAction = menu->addAction("键盘快捷键");
    menu->addSeparator();
    QAction *aboutAction = menu->addAction("关于 IntelNet");
    menuButton_->setMenu(menu);
    toolbar_->addWidget(menuButton_);

    connect(shortcutsAction, &QAction::triggered, this, [this]() {
        QMessageBox::information(this, "键盘快捷键",
            "Alt+← / Alt+→：后退 / 前进\n"
            "F5：刷新\n"
            "Ctrl+Shift+V：打开/关闭语音助手\n"
            "Ctrl+Shift+S：页面摘要\n"
            "Ctrl+Shift+A：分析网页图片\n"
            "Alt+D：描述页面中无 alt 的图片\n"
            "Ctrl+Shift+U：上传本地图片");
    });
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "关于 IntelNet",
            "IntelNet 无障碍浏览器\n版本 1.0.0\n\n"
            "基于 Qt WebEngine (Chromium)\n内置 Qwen2-VL 图像理解与 Piper 语音合成");
    });

    // 快捷键
    QShortcut *voiceShortcut = new QShortcut(QKeySequence("Ctrl+Shift+V"), this);
    connect(voiceShortcut, &QShortcut::activated, this, &MainWindow::onVoiceButtonClicked);

    QShortcut *describeImagesShortcut = new QShortcut(QKeySequence("Alt+D"), this);
    connect(describeImagesShortcut, &QShortcut::activated,
            this, &MainWindow::onDescribeMissingImagesClicked);
}

void MainWindow::setupConnections() {
    // 导航按钮
    connect(backAction_, &QAction::triggered, this, &MainWindow::onBackButtonClicked);
    connect(forwardAction_, &QAction::triggered, this, &MainWindow::onForwardButtonClicked);
    connect(refreshAction_, &QAction::triggered, this, &MainWindow::onRefreshButtonClicked);
    connect(goButton_, &QPushButton::clicked, this, &MainWindow::onGoButtonClicked);
    connect(urlInput_, &QLineEdit::returnPressed, this, &MainWindow::onGoButtonClicked);

    // 浏览器事件
    connect(browserWidget_, &BrowserWidget::urlChanged, this, &MainWindow::onUrlChanged);
    connect(browserWidget_, &BrowserWidget::loadFinished, this, &MainWindow::onLoadFinished);
    connect(browserWidget_, &BrowserWidget::popupCloseButtonDetected,
            this, &MainWindow::onPopupCloseButtonDetected);

    // 语音助手
    connect(voiceButton_, &QPushButton::clicked, this, &MainWindow::onVoiceButtonClicked);
    connect(voicePanel_, &VoicePanel::analyzePageRequested, this, &MainWindow::onAnalyzePageClicked);
    connect(voicePanel_, &VoicePanel::analyzeImageRequested, this, &MainWindow::onAnalyzeImageClicked);
    connect(voicePanel_, &VoicePanel::uploadImageRequested, this, &MainWindow::onUploadImageClicked);
}

void MainWindow::initializeRustCore() {
    rustBridge_ = std::make_unique<RustBridge>();

    if (!rustBridge_->Initialize()) {
        QMessageBox::critical(this, "初始化失败", "Rust 核心库初始化失败！");
        return;
    }

    // 传递 Rust 桥接给语音面板
    voicePanel_->setRustBridge(rustBridge_.get());
}

// ===== 导航槽函数 =====

void MainWindow::onGoButtonClicked() {
    QString url = urlInput_->text().trimmed();
    if (url.isEmpty()) {
        return;
    }

    // 自动补全 URL
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        if (url.contains(".")) {
            url = "https://" + url;
        } else {
            // 搜索
            url = "https://www.baidu.com/s?wd=" + QUrl::toPercentEncoding(url);
        }
    }

    browserWidget_->load(QUrl(url));
    statusBar()->showMessage("正在加载: " + url);
}

void MainWindow::onBackButtonClicked() {
    browserWidget_->back();
}

void MainWindow::onForwardButtonClicked() {
    browserWidget_->forward();
}

void MainWindow::onRefreshButtonClicked() {
    browserWidget_->reload();
    statusBar()->showMessage("刷新页面");
}

void MainWindow::onUrlChanged(const QUrl &url) {
    urlInput_->setText(url.toString());
}

void MainWindow::onLoadFinished(bool ok) {
    if (ok) {
        statusBar()->showMessage("页面加载完成", 3000);
    } else {
        statusBar()->showMessage("页面加载失败", 3000);
    }
}

void MainWindow::onPopupCloseButtonDetected() {
    const QString message = "检测到弹窗，关闭按钮已在右上角标出";
    statusBar()->showMessage(message, 8000);
    if (rustBridge_) rustBridge_->Speak(message.toStdString());
}

// ===== 语音助手槽函数 =====

void MainWindow::showVoicePanel() {
    voicePanel_->show();
    voicePanel_->raise();
    voicePanel_->adjustPosition(this);
}

void MainWindow::onVoiceButtonClicked() {
    if (voicePanel_->isVisible()) {
        voicePanel_->hide();
    } else {
        showVoicePanel();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (voicePanel_ && voicePanel_->isVisible()) {
        voicePanel_->adjustPosition(this);
    }
}

void MainWindow::moveEvent(QMoveEvent *event) {
    QMainWindow::moveEvent(event);
    if (voicePanel_ && voicePanel_->isVisible()) {
        voicePanel_->adjustPosition(this);
    }
}

void MainWindow::onAnalyzePageClicked() {
    showVoicePanel();
    voicePanel_->showBusy("正在读取页面源码...");
    statusBar()->showMessage("正在读取页面源码...", 3000);

    browserWidget_->requestPageSource([this](const QString &source) {
        const QString content = source.trimmed();
        if (content.isEmpty()) {
            voicePanel_->showError("当前页面没有可读取的源码");
            statusBar()->showMessage("当前页面没有可读取的源码", 3000);
            return;
        }

        voicePanel_->showBusy("AI 正在生成页面摘要...");
        statusBar()->showMessage("AI 正在生成页面摘要...", 3000);

        rustBridge_->SummarizeTextAsync(content.toStdString(),
            [this](const std::string &result, bool success) {
                QMetaObject::invokeMethod(this, [this, result, success]() {
                    if (!success) {
                        voicePanel_->showError("页面摘要生成失败");
                        statusBar()->showMessage("页面摘要生成失败", 3000);
                        return;
                    }

                    const QJsonDocument doc =
                        QJsonDocument::fromJson(QByteArray::fromStdString(result));
                    const QString summary = doc.object().value("result").toString();

                    if (summary.isEmpty()) {
                        voicePanel_->showError("页面摘要生成失败");
                        statusBar()->showMessage("页面摘要生成失败", 3000);
                    } else {
                        voicePanel_->showTextResult(summary);
                        statusBar()->showMessage("页面摘要生成完成", 3000);
                    }
                }, Qt::QueuedConnection);
            });
    });
}

void MainWindow::onAnalyzeImageClicked() {
    showVoicePanel();
    voicePanel_->showBusy("请在网页中点击要分析的图片（按 ESC 取消）");
    statusBar()->showMessage("请在网页中点击要分析的图片（按 ESC 取消）", 5000);

    browserWidget_->pickImage([this](const QString &imageData) {
        if (imageData.isEmpty()) {
            voicePanel_->showError("已取消图片选择");
            statusBar()->showMessage("已取消图片选择", 3000);
            return;
        }
        voicePanel_->analyzeImageData(imageData);
        statusBar()->showMessage("正在分析所选图片...", 3000);
    });
}

void MainWindow::onDescribeMissingImagesClicked() {
    if (analyzingMissingImages_) {
        statusBar()->showMessage("正在分析页面图片，请稍候", 3000);
        return;
    }

    showVoicePanel();
    voicePanel_->showBusy("正在查找没有替代文字的图片...");
    statusBar()->showMessage("正在查找没有替代文字的图片...", 3000);

    browserWidget_->requestImagesWithoutAlt([this](
        const QList<BrowserWidget::MissingAltImage> &images) {
        missingAltImages_ = images;
        missingAltIndex_ = 0;

        if (missingAltImages_.isEmpty()) {
            analyzingMissingImages_ = false;
            voicePanel_->showTextResult("当前页面没有找到需要描述的图片。");
            statusBar()->showMessage("当前页面没有找到需要描述的图片", 3000);
            return;
        }

        if (!rustBridge_->InitModel()) {
            voicePanel_->showError("AI 模型初始化失败");
            statusBar()->showMessage("AI 模型初始化失败", 3000);
            return;
        }

        analyzingMissingImages_ = true;
        analyzeNextMissingImage();
    });
}

void MainWindow::analyzeNextMissingImage() {
    if (!analyzingMissingImages_ || missingAltIndex_ >= missingAltImages_.size()) {
        analyzingMissingImages_ = false;
        voicePanel_->showTextResult(QString("已完成 %1 张图片的描述。").arg(missingAltImages_.size()));
        statusBar()->showMessage("页面图片描述完成", 5000);
        return;
    }

    const BrowserWidget::MissingAltImage image = missingAltImages_.at(missingAltIndex_);
    const int displayIndex = missingAltIndex_ + 1;
    const int total = missingAltImages_.size();
    const QString progress = QString("正在分析第 %1/%2 张图片...").arg(displayIndex).arg(total);
    voicePanel_->showBusy(progress);
    statusBar()->showMessage(progress, 0);

    rustBridge_->AnalyzeImageAsync(image.src.toStdString(),
        [this, image, displayIndex, total](const std::string &result, bool success) {
            QMetaObject::invokeMethod(this, [this, image, displayIndex, total, result, success]() {
                if (success) {
                    const QJsonDocument doc =
                        QJsonDocument::fromJson(QByteArray::fromStdString(result));
                    const QString description = doc.object().value("result").toString().trimmed();
                    if (!description.isEmpty()) {
                        browserWidget_->setImageAlt(image.index, "AI描述：" + description);
                        statusBar()->showMessage(
                            QString("第 %1/%2 张图片已写回描述").arg(displayIndex).arg(total), 3000);
                    } else {
                        qWarning() << "图片分析返回空描述:" << image.src;
                    }
                } else {
                    qWarning() << "图片分析失败，跳过:" << image.src;
                }

                ++missingAltIndex_;
                QTimer::singleShot(0, this, &MainWindow::analyzeNextMissingImage);
            }, Qt::QueuedConnection);
        });
}

void MainWindow::onUploadImageClicked() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "选择图片",
        QString(),
        "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"
    );

    if (fileName.isEmpty()) {
        return;
    }

    showVoicePanel();

    // 触发语音面板的图片分析
    voicePanel_->analyzeImageFromFile(fileName);
}

} // namespace IntelNet
