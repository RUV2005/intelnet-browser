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
#include <QUrlQuery>
#include <QScrollArea>
#include <QLabel>
#include <QBuffer>

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
    , unnamedButtonIndex_(0)
    , describedButtons_(0)
    , skippedButtons_(0)
    , describedImages_(0)
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
             "Ctrl+Shift+O：页面大纲\n"
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

    QShortcut *outlineShortcut = new QShortcut(QKeySequence("Ctrl+Shift+O"), this);
    connect(outlineShortcut, &QShortcut::activated, this, &MainWindow::onOutlineRequested);
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
    connect(browserWidget_, &BrowserWidget::redirectChainBlocked,
            this, &MainWindow::onRedirectChainBlocked);
    connect(browserWidget_, &BrowserWidget::warningActionRequested,
            this, &MainWindow::onWarningActionRequested);
    connect(browserWidget_, &BrowserWidget::repeatedAlertBlocked,
            this, &MainWindow::onRepeatedAlertBlocked);

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

    browserWidget_->resetNavigationHistory();
    browserWidget_->load(QUrl(url));
    statusBar()->showMessage("正在加载: " + url);
}

void MainWindow::onBackButtonClicked() {
    browserWidget_->resetNavigationHistory();
    browserWidget_->back();
}

void MainWindow::onForwardButtonClicked() {
    browserWidget_->resetNavigationHistory();
    browserWidget_->forward();
}

void MainWindow::onRefreshButtonClicked() {
    browserWidget_->resetNavigationHistory();
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

void MainWindow::showRedirectWarning(const QUrl &url) {
    QUrl backAction("intelnet://back");
    QUrl continueAction("intelnet://continue");
    QUrlQuery query;
    query.addQueryItem("url", url.toString());
    continueAction.setQuery(query);

    const QString escapedUrl = url.toString().toHtmlEscaped();
    const QString html = QStringLiteral(R"HTML(
<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8"><title>已拦截可疑跳转</title>
<style>
body{margin:0;background:#08090c;color:#f7f7f8;font-family:system-ui,sans-serif;display:grid;place-items:center;min-height:100vh}
main{width:min(680px,calc(100% - 48px));padding:42px;border:1px solid #3b3d4a;border-radius:20px;background:#11131a;box-shadow:0 20px 60px #0008}
h1{font-size:30px;margin:0 0 16px;color:#ff8b8b}p{line-height:1.7;color:#c6c7d0}code{display:block;word-break:break-all;padding:14px;border-radius:8px;background:#08090c;color:#b7f3ff}
.actions{display:flex;gap:12px;margin-top:28px}a{padding:12px 18px;border-radius:8px;text-decoration:none;font-weight:600}a:first-child{background:#7566ff;color:white}a:last-child{border:1px solid #555866;color:#f7f7f8}
</style></head><body><main>
<h1>检测到可疑连环跳转，已拦截</h1>
<p>页面在短时间内连续跳转到多个不同网站。为保护你的安全，浏览器已暂停这次跳转。</p>
<p>目标地址：</p><code>%1</code>
<div class="actions"><a href="%2">返回上一页</a><a href="%3">仍然继续</a></div>
</main></body></html>
)HTML").arg(escapedUrl,
              QString::fromLatin1(backAction.toEncoded()),
              QString::fromLatin1(continueAction.toEncoded()));
    browserWidget_->setHtml(html, QUrl("https://intelnet-warning.local/"));
}

void MainWindow::onRedirectChainBlocked(const QUrl &url) {
    const QString message = "检测到可疑连环跳转，已拦截";
    showRedirectWarning(url);
    statusBar()->showMessage(message, 8000);
    if (rustBridge_) rustBridge_->Speak(message.toStdString());
}

void MainWindow::onWarningActionRequested(const QString &action, const QUrl &url) {
    if (action == "back") {
        browserWidget_->back();
    } else if (action == "continue" && url.isValid() && !url.isEmpty()) {
        browserWidget_->allowNavigationOnce(url);
        browserWidget_->load(url);
    }
}

void MainWindow::onRepeatedAlertBlocked() {
    const QString message = "已屏蔽重复弹窗";
    statusBar()->showMessage(message, 5000);
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

void MainWindow::onOutlineRequested() {
    showVoicePanel();
    voicePanel_->showBusy("正在提取页面标题...");
    statusBar()->showMessage("正在提取页面标题...", 3000);

    browserWidget_->requestHeadingOutline([this](
        const QList<BrowserWidget::HeadingEntry> &headings) {
        if (headings.isEmpty()) {
            const QString message = "本页没有可用的标题结构";
            voicePanel_->showTextResult(message);
            statusBar()->showMessage(message, 5000);
            return;
        }

        const QString message = QString("已生成页面大纲，共 %1 个标题").arg(headings.size());
        voicePanel_->showTextResult(message);
        statusBar()->showMessage(message, 5000);
        showHeadingOutline(headings);
    });
}

void MainWindow::showHeadingOutline(
    const QList<BrowserWidget::HeadingEntry> &headings) {
    QDialog dialog(this);
    dialog.setWindowTitle("页面大纲");
    dialog.setMinimumSize(520, 620);

    QVBoxLayout *root = new QVBoxLayout(&dialog);
    QLabel *hint = new QLabel("选择标题跳转到对应章节", &dialog);
    hint->setAccessibleName("页面大纲说明");
    root->addWidget(hint);

    QScrollArea *scroll = new QScrollArea(&dialog);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *content = new QWidget(scroll);
    QVBoxLayout *items = new QVBoxLayout(content);
    items->setContentsMargins(4, 4, 12, 4);
    items->setSpacing(6);

    int firstLevel = headings.first().level;
    int previousDepth = 0;
    for (int i = 0; i < headings.size(); ++i) {
        const auto &heading = headings.at(i);
        const int relativeLevel = qMax(0, heading.level - firstLevel);
        const int depth = i == 0 ? 0 : qMin(relativeLevel, previousDepth + 1);
        previousDepth = depth;

        QPushButton *button = new QPushButton(heading.text, content);
        button->setAccessibleName(QString("第 %1 级标题：%2").arg(depth + 1).arg(heading.text));
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(QString("text-align:left;padding:9px 12px 9px %1px;").arg(12 + depth * 24));
        connect(button, &QPushButton::clicked, &dialog, [this, &dialog, id = heading.id]() {
            browserWidget_->scrollToHeading(id);
            dialog.accept();
        });
        items->addWidget(button);
    }
    items->addStretch();
    content->setLayout(items);
    scroll->setWidget(content);
    root->addWidget(scroll);
    dialog.exec();
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
        describedImages_ = 0;

        if (missingAltImages_.isEmpty()) {
            requestUnnamedButtonsAfterImages();
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
        requestUnnamedButtonsAfterImages();
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
                        ++describedImages_;
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

void MainWindow::requestUnnamedButtonsAfterImages() {
    analyzingMissingImages_ = true;
    voicePanel_->showBusy("正在查找没有语义的按钮...");
    statusBar()->showMessage("正在查找没有语义的按钮...", 3000);
    browserWidget_->requestUnnamedButtons([this](
        const QList<BrowserWidget::UnnamedButton> &buttons) {
        unnamedButtons_ = buttons;
        unnamedButtonIndex_ = 0;
        describedButtons_ = 0;
        skippedButtons_ = 0;
        constexpr int maxButtonsPerRun = 10;
        if (unnamedButtons_.size() > maxButtonsPerRun) {
            skippedButtons_ = unnamedButtons_.size() - maxButtonsPerRun;
            unnamedButtons_ = unnamedButtons_.mid(0, maxButtonsPerRun);
            statusBar()->showMessage(
                QString("发现 %1 个无语义按钮，本次最多处理 %2 个，剩余 %3 个跳过")
                    .arg(buttons.size()).arg(maxButtonsPerRun).arg(skippedButtons_), 6000);
        }
        if (unnamedButtons_.isEmpty()) {
            analyzingMissingImages_ = false;
            const QString message = QString("已完成 %1 张图片的描述，未发现无语义按钮。").arg(describedImages_);
            voicePanel_->showTextResult(message);
            statusBar()->showMessage(message, 5000);
            return;
        }
        analyzeNextUnnamedButton();
    });
}

void MainWindow::analyzeNextUnnamedButton() {
    if (unnamedButtonIndex_ >= unnamedButtons_.size()) {
        analyzingMissingImages_ = false;
        const QString message = skippedButtons_ > 0
            ? QString("已为 %1 个按钮生成描述，另有 %2 个按钮未处理。")
                  .arg(describedButtons_).arg(skippedButtons_)
            : QString("已为 %1 个按钮生成描述。").arg(describedButtons_);
        voicePanel_->showTextResult(message);
        statusBar()->showMessage(message, 5000);
        return;
    }

    const BrowserWidget::UnnamedButton button = unnamedButtons_.at(unnamedButtonIndex_);
    const int displayIndex = unnamedButtonIndex_ + 1;
    const int total = unnamedButtons_.size();
    const QString progress = QString("正在分析第 %1/%2 个按钮...").arg(displayIndex).arg(total);
    voicePanel_->showBusy(progress);
    statusBar()->showMessage(progress, 0);

    browserWidget_->captureElement(button.id, [this, button](const QImage &image) {
        if (image.isNull()) {
            ++unnamedButtonIndex_;
            QTimer::singleShot(0, this, &MainWindow::analyzeNextUnnamedButton);
            return;
        }
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "JPEG", 80);
        const std::string data = ("data:image/jpeg;base64," + bytes.toBase64()).toStdString();
        rustBridge_->DescribeButtonAsync(data,
            [this, button](const std::string &result, bool success) {
                QMetaObject::invokeMethod(this, [this, button, result, success]() {
                    if (success) {
                        const QJsonDocument doc = QJsonDocument::fromJson(
                            QByteArray::fromStdString(result));
                        const QString label = doc.object().value("result").toString().trimmed();
                        if (!label.isEmpty()) {
                            browserWidget_->setButtonAriaLabel(button.id, label);
                            ++describedButtons_;
                        }
                    }
                    ++unnamedButtonIndex_;
                    QTimer::singleShot(0, this, &MainWindow::analyzeNextUnnamedButton);
                }, Qt::QueuedConnection);
            });
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
