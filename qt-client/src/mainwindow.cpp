// 主窗口实现
#include "mainwindow.h"
#include "theme.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QShortcut>
#include <QStatusBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QDebug>
#include <QTimer>
#include <QUrlQuery>
#include <QScrollArea>
#include <QLabel>
#include <QBuffer>
#include <QWebEngineHistory>

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
    , voiceDock_(nullptr)
    , rustBridge_(nullptr)
    , modelDownloader_(new ModelDownloader(this))
    , modelProgressDialog_(nullptr)
    , modelsReady_(false)
    , modelDownloadStarted_(false)
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

    connect(modelDownloader_, &ModelDownloader::progress, this,
            [this](qint64 downloaded, qint64 total, const QString &fileName) {
        if (!modelProgressDialog_) return;
        if (total > 0) {
            modelProgressDialog_->setRange(0, 1000);
            modelProgressDialog_->setValue(qBound(0, static_cast<int>((downloaded * 1000) / total), 1000));
        } else {
            modelProgressDialog_->setRange(0, 0);
        }
        if (!fileName.isEmpty()) {
            modelProgressDialog_->setLabelText(QString("正在下载模型：%1").arg(fileName));
        }
    });
    connect(modelDownloader_, &ModelDownloader::completed, this, &MainWindow::onModelsReady);
    connect(modelDownloader_, &ModelDownloader::failed, this, &MainWindow::onModelDownloadFailed);
    connect(modelDownloader_, &ModelDownloader::cancelled, this, &MainWindow::onModelDownloadCancelled);

    // 设置窗口属性
    setWindowTitle("IntelNet 无障碍浏览器");
    resize(1200, 800);
    statusBar()->showMessage("就绪");
    QTimer::singleShot(0, this, &MainWindow::checkModelsAndStart);
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

    // 创建语音助手面板（主窗口右侧停靠）
    voicePanel_ = new VoicePanel(this);
    voiceDock_ = new QDockWidget("语音助手", this);
    voiceDock_->setObjectName("VoiceDock");
    voiceDock_->setAllowedAreas(Qt::RightDockWidgetArea);
    voiceDock_->setFeatures(QDockWidget::NoDockWidgetFeatures);
    voiceDock_->setFocusPolicy(Qt::StrongFocus);
    auto *dockTitleBar = new QWidget(voiceDock_);
    dockTitleBar->setFixedHeight(0);
    voiceDock_->setTitleBarWidget(dockTitleBar);
    voiceDock_->setWidget(voicePanel_);
    addDockWidget(Qt::RightDockWidgetArea, voiceDock_);
    voiceDock_->hide();

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
             "Ctrl+Shift+C：识别验证码\n"
             "Ctrl+Shift+L：播放音频验证码\n"
             "Ctrl+Shift+F：解释当前表单\n"
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
    voiceShortcut->setContext(Qt::ApplicationShortcut);
    connect(voiceShortcut, &QShortcut::activated, this, &MainWindow::onVoiceButtonClicked);

    QShortcut *describeImagesShortcut = new QShortcut(QKeySequence("Alt+D"), this);
    describeImagesShortcut->setContext(Qt::ApplicationShortcut);
    connect(describeImagesShortcut, &QShortcut::activated,
            this, &MainWindow::onDescribeMissingImagesClicked);

    QShortcut *outlineShortcut = new QShortcut(QKeySequence("Ctrl+Shift+O"), this);
    outlineShortcut->setContext(Qt::ApplicationShortcut);
    connect(outlineShortcut, &QShortcut::activated, this, &MainWindow::onOutlineRequested);

    QShortcut *captchaShortcut = new QShortcut(QKeySequence("Ctrl+Shift+C"), this);
    captchaShortcut->setContext(Qt::ApplicationShortcut);
    connect(captchaShortcut, &QShortcut::activated, this, &MainWindow::onCaptchaRequested);
    QShortcut *audioCaptchaShortcut = new QShortcut(QKeySequence("Ctrl+Shift+L"), this);
    audioCaptchaShortcut->setContext(Qt::ApplicationShortcut);
    connect(audioCaptchaShortcut, &QShortcut::activated, this, &MainWindow::onAudioCaptchaRequested);
    QShortcut *formShortcut = new QShortcut(QKeySequence("Ctrl+Shift+F"), this);
    formShortcut->setContext(Qt::ApplicationShortcut);
    connect(formShortcut, &QShortcut::activated, this, &MainWindow::onFormRequested);

    QShortcut *summaryShortcut = new QShortcut(QKeySequence("Ctrl+Shift+S"), this);
    summaryShortcut->setContext(Qt::ApplicationShortcut);
    connect(summaryShortcut, &QShortcut::activated, this, &MainWindow::onAnalyzePageClicked);
    QShortcut *imageShortcut = new QShortcut(QKeySequence("Ctrl+Shift+A"), this);
    imageShortcut->setContext(Qt::ApplicationShortcut);
    connect(imageShortcut, &QShortcut::activated, this, &MainWindow::onAnalyzeImageClicked);
    QShortcut *uploadShortcut = new QShortcut(QKeySequence("Ctrl+Shift+U"), this);
    uploadShortcut->setContext(Qt::ApplicationShortcut);
    connect(uploadShortcut, &QShortcut::activated, this, &MainWindow::onUploadImageClicked);
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
    backAction_->setEnabled(true);
    forwardAction_->setEnabled(true);
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
    connect(voicePanel_, &VoicePanel::captchaRequested, this, &MainWindow::onCaptchaRequested);
    connect(voicePanel_, &VoicePanel::audioCaptchaRequested, this, &MainWindow::onAudioCaptchaRequested);
    connect(voicePanel_, &VoicePanel::formRequested, this, &MainWindow::onFormRequested);
    connect(voicePanel_, &VoicePanel::panelCloseRequested, voiceDock_, &QDockWidget::hide);
    connect(browserWidget_, &BrowserWidget::formSubmitIntercepted,
            this, &MainWindow::onFormSubmitIntercepted);
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

void MainWindow::checkModelsAndStart() {
    if (ModelDownloader::modelsPresent()) {
        onModelsReady();
        return;
    }

    modelProgressDialog_ = new QProgressDialog(
        "首次启动需要下载 AI 模型，约 3.5GB", "开始下载", 0, 1000, this);
    modelProgressDialog_->setWindowTitle("下载 AI 模型");
    modelProgressDialog_->setWindowModality(Qt::ApplicationModal);
    modelProgressDialog_->setAutoClose(false);
    modelProgressDialog_->setAutoReset(false);
    modelProgressDialog_->setMinimumDuration(0);
    modelProgressDialog_->setAccessibleName("AI 模型下载");
    modelProgressDialog_->setAccessibleDescription(
        "首次启动需要下载 AI 模型，约 3.5GB。按开始下载开始，按取消停止下载。");
    connect(modelProgressDialog_, &QProgressDialog::canceled, this, [this]() {
        if (!modelDownloadStarted_) {
            startModelDownload();
        } else {
            modelDownloader_->cancel();
        }
    });
    modelProgressDialog_->show();
}

void MainWindow::startModelDownload() {
    if (!modelProgressDialog_) return;
    modelDownloadStarted_ = true;
    modelProgressDialog_->setCancelButtonText("取消");
    modelProgressDialog_->setLabelText("正在准备下载 AI 模型...");
    modelProgressDialog_->setRange(0, 0);
    modelDownloader_->start();
}

void MainWindow::onModelsReady() {
    modelsReady_ = true;
    if (modelProgressDialog_) {
        modelProgressDialog_->close();
        modelProgressDialog_->deleteLater();
        modelProgressDialog_ = nullptr;
    }
    initializeRustCore();
}

void MainWindow::onModelDownloadFailed(const QString &message) {
    if (modelProgressDialog_) modelProgressDialog_->setCancelButtonText("重试");
    const auto choice = QMessageBox::critical(this, "模型下载失败", message,
                                               QMessageBox::Retry | QMessageBox::Cancel,
                                               QMessageBox::Retry);
    if (choice == QMessageBox::Retry) {
        startModelDownload();
    } else {
        onModelDownloadCancelled();
    }
}

void MainWindow::onModelDownloadCancelled() {
    if (modelProgressDialog_) {
        modelProgressDialog_->close();
        modelProgressDialog_->deleteLater();
        modelProgressDialog_ = nullptr;
    }
    statusBar()->showMessage("AI 模型未下载，AI 功能暂不可用");
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
    statusBar()->showMessage(QString("后退：历史 %1 项，当前位置 %2，可后退=%3")
        .arg(browserWidget_->history()->count())
        .arg(browserWidget_->history()->currentItemIndex())
        .arg(browserWidget_->history()->canGoBack() ? "是" : "否"), 5000);
    browserWidget_->back();
}

void MainWindow::onForwardButtonClicked() {
    browserWidget_->resetNavigationHistory();
    statusBar()->showMessage(QString("前进：历史 %1 项，当前位置 %2，可前进=%3")
        .arg(browserWidget_->history()->count())
        .arg(browserWidget_->history()->currentItemIndex())
        .arg(browserWidget_->history()->canGoForward() ? "是" : "否"), 5000);
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
    qDebug() << "load finished" << ok << browserWidget_->url()
             << "history" << browserWidget_->history()->count()
             << "index" << browserWidget_->history()->currentItemIndex();
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
    voiceDock_->show();
    voiceDock_->raise();
    voiceDock_->setFocus();
}

void MainWindow::onVoiceButtonClicked() {
    if (voiceDock_->isVisible()) {
        voiceDock_->hide();
    } else {
        showVoicePanel();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
}

void MainWindow::moveEvent(QMoveEvent *event) {
    QMainWindow::moveEvent(event);
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
                        browserWidget_->setImageAltById(image.id, "AI描述：" + description);
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
                    .arg(static_cast<int>(buttons.size())).arg(maxButtonsPerRun).arg(skippedButtons_), 6000);
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

void MainWindow::onCaptchaRequested() {
    showVoicePanel();
    voicePanel_->showBusy("正在查找验证码...");
    statusBar()->showMessage("正在查找验证码...", 3000);
    browserWidget_->requestCaptchaCandidate([this](const BrowserWidget::CaptchaCandidate &candidate) {
        if (candidate.id.isEmpty()) {
            const QString message = "当前页面没有找到验证码图片";
            voicePanel_->showTextResult(message);
            statusBar()->showMessage(message, 5000);
            return;
        }
        voicePanel_->showBusy("正在加载 OCR 模型并识别验证码...");
        statusBar()->showMessage("正在加载 OCR 模型并识别验证码...", 0);
        browserWidget_->captureElement(candidate.id, [this](const QImage &image) {
            if (image.isNull()) {
                voicePanel_->showError("无法截取验证码图片");
                statusBar()->showMessage("无法截取验证码图片", 5000);
                return;
            }
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "JPEG", 85);
            const std::string data = ("data:image/jpeg;base64," + bytes.toBase64()).toStdString();
            rustBridge_->OcrCaptchaAsync(data,
                [this](const std::string &result, bool success) {
                    QMetaObject::invokeMethod(this, [this, result, success]() {
                        if (!success) {
                            voicePanel_->showError("验证码识别失败");
                            statusBar()->showMessage("验证码识别失败", 5000);
                            return;
                        }
                        const QJsonDocument doc = QJsonDocument::fromJson(
                            QByteArray::fromStdString(result));
                        const QString text = doc.object().value("result").toString().trimmed();
                        if (text.isEmpty()) {
                            voicePanel_->showError("未识别到验证码");
                            statusBar()->showMessage("未识别到验证码", 5000);
                            return;
                        }
                        voicePanel_->showTextResult(text, false);
                        statusBar()->showMessage("验证码识别完成", 5000);
                    }, Qt::QueuedConnection);
                });
        });
    });
}

void MainWindow::onAudioCaptchaRequested() {
    showVoicePanel();
    voicePanel_->showBusy("正在切换并播放音频验证码...");
    statusBar()->showMessage("正在切换并播放音频验证码...", 5000);
    browserWidget_->playAudioCaptcha([this](bool playing) {
        if (playing) {
            voicePanel_->showBusy("音频验证码播放中...");
            statusBar()->showMessage("音频验证码播放中", 5000);
        } else {
            voicePanel_->showError("未找到音频验证码，或浏览器阻止了自动播放");
            statusBar()->showMessage("未找到音频验证码，或浏览器阻止了自动播放", 8000);
        }
    });
}

void MainWindow::onFormRequested() {
    showVoicePanel();
    voicePanel_->showBusy("正在读取当前表单...");
    statusBar()->showMessage("正在读取当前表单...", 3000);
    browserWidget_->requestFormStructure([this](const QString &structure) {
        if (structure.trimmed().isEmpty() || structure == "[]") {
            voicePanel_->showTextResult("当前页面没有找到可解释的表单。");
            statusBar()->showMessage("当前页面没有找到可解释的表单", 5000);
            return;
        }
        voicePanel_->showBusy("正在解释表单...");
        rustBridge_->ExplainFormAsync(structure.toStdString(),
            [this](const std::string &result, bool success) {
                QMetaObject::invokeMethod(this, [this, result, success]() {
                    if (!success) {
                        voicePanel_->showError("表单解释失败");
                        statusBar()->showMessage("表单解释失败", 5000);
                        return;
                    }
                    const QJsonDocument doc = QJsonDocument::fromJson(
                        QByteArray::fromStdString(result));
                    const QString text = doc.object().value("result").toString().trimmed();
                    if (text.isEmpty()) {
                        voicePanel_->showError("表单解释为空");
                        return;
                    }
                    voicePanel_->showTextResult(text, false);
                    statusBar()->showMessage("表单解释完成", 5000);
                }, Qt::QueuedConnection);
            });
    });
}

void MainWindow::onFormSubmitIntercepted(const QString &payload) {
    const QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
    if (!doc.isObject()) return;
    const QJsonObject object = doc.object();
    const QJsonArray fields = object.value("fields").toArray();
    QStringList missing;
    int firstMissing = -1;
    QStringList values;
    for (const QJsonValue &value : fields) {
        const QJsonObject field = value.toObject();
        const QString label = field.value("label").toString().trimmed();
        const QString type = field.value("type").toString();
        const QString fieldValue = field.value("value").toString();
        if (field.value("required").toBool() && fieldValue.trimmed().isEmpty()) {
            missing << label;
            if (firstMissing < 0) firstMissing = field.value("fieldIndex").toInt(-1);
        }
        if (type == "password") {
            values << QString("%1已填写").arg(label);
        } else if (!fieldValue.trimmed().isEmpty()) {
            values << QString("%1%2").arg(label, fieldValue.trimmed());
        }
    }

    if (!missing.isEmpty()) {
        browserWidget_->cancelFormConfirmation();
        const QString message = QString("还有 %1 项没填：%2")
            .arg(static_cast<int>(missing.size()))
            .arg(missing.join("、"));
        voicePanel_->showTextResult(message);
        statusBar()->showMessage(message, 8000);
        if (firstMissing >= 0) {
            browserWidget_->focusFormField(object.value("formIndex").toInt(-1), firstMissing);
        }
        return;
    }

    const QString message = QString("请确认，%1，按回车提交").arg(values.join("，"));
    browserWidget_->armFormConfirmation();
    voicePanel_->showTextResult(message);
    statusBar()->showMessage("表单待确认，按回车提交", 8000);
}

} // namespace IntelNet
