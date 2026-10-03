// 简化版无障碍工具栏演示
#include "simple_accessible_toolbar.h"
#include <QApplication>
#include <QMainWindow>
#include <QWebEngineView>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>

using namespace IntelNet;

class SimpleDemoWindow : public QMainWindow {
    Q_OBJECT

public:
    SimpleDemoWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("IntelNet Browser - 无障碍设计演示");
        resize(1400, 900);

        // 创建中心部件
        auto *centralWidget = new QWidget(this);
        auto *layout = new QVBoxLayout(centralWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // 创建工具栏
        toolbar_ = new SimpleAccessibleToolbar(this);
        layout->addWidget(toolbar_);

        // 创建浏览器
        webView_ = new QWebEngineView(this);
        layout->addWidget(webView_, 1);

        setCentralWidget(centralWidget);

        // 连接信号
        connectSignals();

        // 加载演示页面
        loadDemoPage();

        // 设置页面信息
        toolbar_->setPageInfo("人工智能的最新进展", "科技日报", 5);

        // 2秒后显示AI建议
        QTimer::singleShot(2000, this, [this]() {
            toolbar_->setAIUnreadCount(3);
        });
    }

private:
    void connectSignals() {
        // 语音输入
        connect(toolbar_, &SimpleAccessibleToolbar::voiceClicked, this, [this]() {
            QMessageBox::information(this, "🎤 语音输入",
                "语音输入已激活！\n\n"
                "📌 支持的命令：\n"
                "• \"朗读这篇文章\"\n"
                "• \"总结页面内容\"\n"
                "• \"翻译成英文\"\n"
                "• \"切换到阅读模式\"\n\n"
                "请说话...");
        });

        // AI 助手
        connect(toolbar_, &SimpleAccessibleToolbar::aiAssistantClicked, this, [this]() {
            QMessageBox::information(this, "🤖 AI 助手",
                "💡 智能建议 (3条)：\n\n"
                "1️⃣ 这篇文章较长 (约5分钟)，需要我总结吗？\n\n"
                "2️⃣ 检测到专业术语，需要解释吗？\n\n"
                "3️⃣ 发现相关图片，需要描述内容吗？");
            toolbar_->setAIUnreadCount(0);
        });

        // 朗读控制
        connect(toolbar_, &SimpleAccessibleToolbar::playPauseClicked, this, [this]() {
            isReading_ = !isReading_;
            toolbar_->setReadingState(isReading_);

            if (isReading_) {
                toolbar_->showReadingControls();
                startReading();
            } else {
                toolbar_->hideReadingControls();
                stopReading();
            }
        });

        connect(toolbar_, &SimpleAccessibleToolbar::nextParagraphClicked, this, [this]() {
            if (currentParagraph_ < totalParagraphs_) {
                currentParagraph_++;
                toolbar_->setReadingProgress(currentParagraph_, totalParagraphs_);
            }
        });

        connect(toolbar_, &SimpleAccessibleToolbar::previousParagraphClicked, this, [this]() {
            if (currentParagraph_ > 1) {
                currentParagraph_--;
                toolbar_->setReadingProgress(currentParagraph_, totalParagraphs_);
            }
        });

        // 快捷操作
        connect(toolbar_, &SimpleAccessibleToolbar::readingModeClicked, this, [this]() {
            QMessageBox::information(this, "📖 阅读模式",
                "✅ 已切换到阅读模式\n\n"
                "优化内容：\n"
                "• 去除广告和侧边栏\n"
                "• 简化页面布局\n"
                "• 增大字体和行距\n"
                "• 使用高对比度配色");
        });

        connect(toolbar_, &SimpleAccessibleToolbar::summarizeClicked, this, [this]() {
            QMessageBox::information(this, "📝 页面总结",
                "🤖 AI 生成的总结：\n\n"
                "这是一篇关于人工智能在无障碍领域应用的报道。\n"
                "文章介绍了语音识别、图像描述等技术的最新进展，\n"
                "以及这些技术如何帮助视障、听障用户更好地使用互联网。\n\n"
                "关键词：AI、无障碍、语音识别、图像描述、自然语言处理");
        });

        connect(toolbar_, &SimpleAccessibleToolbar::translateClicked, this, [this]() {
            QMessageBox::information(this, "🌐 翻译",
                "选择目标语言：\n\n"
                "• English (英语)\n"
                "• 日本語 (日语)\n"
                "• 한국어 (韩语)\n"
                "• Français (法语)\n"
                "• Deutsch (德语)");
        });

        connect(toolbar_, &SimpleAccessibleToolbar::bookmarkClicked, this, [this]() {
            QMessageBox::information(this, "⭐ 书签",
                "✅ 已添加到书签\n\n"
                "📁 我的书签\n"
                "└─ 科技新闻\n"
                "   └─ 人工智能的最新进展 ✨");
        });
    }

    void loadDemoPage() {
        QString html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>人工智能的最新进展</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: "Microsoft YaHei", "PingFang SC", Arial, sans-serif;
            background: #F5F7FA;
            color: #333;
            line-height: 1.8;
        }
        .container {
            max-width: 900px;
            margin: 0 auto;
            padding: 40px 20px;
            background: white;
            min-height: 100vh;
        }
        header {
            border-bottom: 3px solid #0066FF;
            padding-bottom: 20px;
            margin-bottom: 30px;
        }
        h1 {
            color: #0066FF;
            font-size: 36px;
            margin-bottom: 15px;
            font-weight: bold;
        }
        .meta {
            color: #666;
            font-size: 14px;
            display: flex;
            gap: 20px;
            flex-wrap: wrap;
        }
        .meta-item {
            display: flex;
            align-items: center;
            gap: 5px;
        }
        .content {
            font-size: 18px;
        }
        .highlight {
            background: linear-gradient(to bottom, transparent 50%, #FFE066 50%);
            padding: 2px 0;
            font-weight: bold;
        }
        p {
            margin-bottom: 24px;
            text-align: justify;
            text-indent: 2em;
        }
        strong {
            color: #0066FF;
            font-weight: bold;
        }
        .quote {
            border-left: 4px solid #0066FF;
            padding-left: 20px;
            margin: 30px 0;
            font-style: italic;
            color: #555;
            background: #F8F9FA;
            padding: 20px;
            border-radius: 0 8px 8px 0;
        }
        .section {
            margin-top: 40px;
        }
        h2 {
            color: #0066FF;
            font-size: 24px;
            margin-bottom: 20px;
            padding-bottom: 10px;
            border-bottom: 2px solid #E0E0E0;
        }
        .info-box {
            background: #E8F4FF;
            border: 2px solid #0066FF;
            border-radius: 12px;
            padding: 20px;
            margin: 30px 0;
        }
        .info-box h3 {
            color: #0066FF;
            margin-bottom: 10px;
        }
        ul {
            margin-left: 2em;
            margin-bottom: 20px;
        }
        li {
            margin-bottom: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>人工智能的最新进展</h1>
            <div class="meta">
                <span class="meta-item">📰 来源：科技日报</span>
                <span class="meta-item">📅 2024年3月15日</span>
                <span class="meta-item">⏱️ 约5分钟阅读</span>
                <span class="meta-item">👁️ 已阅读 2,345 次</span>
            </div>
        </header>

        <div class="content">
            <p>
                <span class="highlight">人工智能技术</span>正在以前所未有的速度发展，
                尤其在<strong>无障碍技术</strong>领域取得了突破性进展。
                最新的大语言模型能够理解复杂的自然语言指令，
                为视障、听障和行动不便的用户提供了全新的交互方式。
            </p>

            <p>
                语音识别技术的准确率已经超过<strong>98%</strong>，
                即使在嘈杂环境中也能准确识别用户的指令。
                结合自然语言处理技术，AI助手可以理解用户的真实意图，
                而不仅仅是机械地执行预设的命令。
            </p>

            <div class="quote">
                "技术的真正价值在于让每个人都能平等地享受数字世界的精彩。
                无障碍不应该是可选项，而应该是标准配置。"
                <br>—— 某知名无障碍技术专家
            </div>

            <div class="section">
                <h2>🎯 核心技术突破</h2>

                <p>
                    图像识别技术同样取得了重大突破。现代AI系统不仅能够识别图片中的物体，
                    还能理解场景的上下文，生成自然流畅的描述。
                    这对于视障用户浏览网页内容具有<span class="highlight">革命性的意义</span>。
                </p>

                <div class="info-box">
                    <h3>💡 技术亮点</h3>
                    <ul>
                        <li><strong>语音识别</strong>：支持方言、口音，准确率98%+</li>
                        <li><strong>图像描述</strong>：理解上下文，生成自然语言</li>
                        <li><strong>实时翻译</strong>：支持100+语言互译</li>
                        <li><strong>智能总结</strong>：自动提取关键信息</li>
                    </ul>
                </div>
            </div>

            <div class="section">
                <h2>🌟 实际应用场景</h2>

                <p>
                    更重要的是，这些技术正在变得越来越普及和易用。
                    开发者可以轻松地将无障碍功能集成到他们的应用中，
                    而用户也能以更低的成本享受到这些技术带来的便利。
                </p>

                <p>
                    在教育领域，AI驱动的无障碍工具帮助残障学生更好地学习；
                    在就业市场，这些技术让更多残障人士获得了工作机会；
                    在日常生活中，智能助手成为了视障用户的"数字眼睛"。
                </p>
            </div>

            <div class="section">
                <h2>🚀 未来展望</h2>

                <p>
                    展望未来，AI技术将继续在无障碍领域发挥重要作用。
                    我们期待看到更多创新的应用，让每个人都能平等地享受数字世界的精彩。
                </p>

                <p>
                    下一代无障碍技术将更加智能、更加个性化。
                    它们不仅能理解用户的需求，还能预测用户的意图，
                    提供主动式的帮助。这将彻底改变残障人士与数字世界的互动方式。
                </p>
            </div>
        </div>
    </div>
</body>
</html>
        )";

        webView_->setHtml(html);
    }

    void startReading() {
        totalParagraphs_ = 12;
        currentParagraph_ = 1;
        toolbar_->setReadingProgress(currentParagraph_, totalParagraphs_);
        toolbar_->setReadingSpeed(1.0);

        readingTimer_ = new QTimer(this);
        connect(readingTimer_, &QTimer::timeout, this, [this]() {
            if (isReading_ && currentParagraph_ < totalParagraphs_) {
                currentParagraph_++;
                toolbar_->setReadingProgress(currentParagraph_, totalParagraphs_);
            } else if (currentParagraph_ >= totalParagraphs_) {
                stopReading();
                toolbar_->setReadingState(false);
                QMessageBox::information(this, "✅ 朗读完成", "已朗读完全部内容！");
            }
        });
        readingTimer_->start(2000); // 每2秒一段
    }

    void stopReading() {
        if (readingTimer_) {
            readingTimer_->stop();
            readingTimer_->deleteLater();
            readingTimer_ = nullptr;
        }
    }

    SimpleAccessibleToolbar *toolbar_;
    QWebEngineView *webView_;
    QTimer *readingTimer_ = nullptr;
    int currentParagraph_ = 0;
    int totalParagraphs_ = 0;
    bool isReading_ = false;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("IntelNet Browser");
    app.setOrganizationName("IntelNet");

    SimpleDemoWindow window;
    window.show();

    return app.exec();
}

#include "simple_demo.moc"
