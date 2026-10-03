// 无障碍工具栏演示程序
#include <QApplication>
#include <QMainWindow>
#include <QWebEngineView>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include "accessible_toolbar.h"

using namespace IntelNet;

class DemoWindow : public QMainWindow {
    Q_OBJECT

public:
    DemoWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("IntelNet Browser - 无障碍演示");
        resize(1200, 800);

        // 创建中心部件
        auto *centralWidget = new QWidget(this);
        auto *layout = new QVBoxLayout(centralWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // 创建无障碍工具栏
        toolbar_ = new AccessibleToolbar(this);
        layout->addWidget(toolbar_);

        // 创建浏览器视图
        webView_ = new QWebEngineView(this);
        layout->addWidget(webView_, 1);

        setCentralWidget(centralWidget);

        // 连接信号
        connectSignals();

        // 加载示例页面
        loadDemoPage();

        // 设置初始状态
        toolbar_->setPageInfo(
            "人工智能的最新进展",
            "科技日报",
            5
        );

        // 模拟 AI 建议
        QTimer::singleShot(2000, this, [this]() {
            toolbar_->setAIUnreadCount(3);
        });
    }

private:
    void connectSignals() {
        // 语音相关
        connect(toolbar_, &AccessibleToolbar::voicePressAndHold, this, [this]() {
            QMessageBox::information(this, "语音输入", "按住说话模式激活！\n\n请说话...");
        });

        connect(toolbar_, &AccessibleToolbar::voiceToggleMode, this, [this]() {
            QMessageBox::information(this, "语音输入", "长对话模式切换！\n\n现在可以连续提问。");
        });

        // 朗读控制
        connect(toolbar_, &AccessibleToolbar::readingTogglePlayPause, this, [this]() {
            static bool isPlaying = false;
            isPlaying = !isPlaying;
            toolbar_->setReadingState(isPlaying);

            if (isPlaying) {
                toolbar_->showReadingControls();
                simulateReading();
            } else {
                toolbar_->hideReadingControls();
            }
        });

        connect(toolbar_, &AccessibleToolbar::readingNext, this, [this]() {
            currentParagraph_ = qMin(currentParagraph_ + 1, totalParagraphs_);
            toolbar_->setReadingProgress(currentParagraph_, totalParagraphs_);
        });

        connect(toolbar_, &AccessibleToolbar::readingPrevious, this, [this]() {
            currentParagraph_ = qMax(currentParagraph_ - 1, 1);
            toolbar_->setReadingProgress(currentParagraph_, totalParagraphs_);
        });

        connect(toolbar_, &AccessibleToolbar::readingSpeedChanged, this, [](qreal speed) {
            qDebug() << "朗读速度变更为:" << speed << "x";
        });

        // AI 助手
        connect(toolbar_, &AccessibleToolbar::aiAssistantRequested, this, [this]() {
            QMessageBox::information(this, "AI 助手",
                "💡 智能建议：\n\n"
                "1. 这篇文章较长，要总结吗？\n"
                "2. 检测到技术术语，要解释吗？\n"
                "3. 有相关图片，要描述吗？");
        });

        // 快捷操作
        connect(toolbar_, &AccessibleToolbar::readingModeRequested, this, [this]() {
            QMessageBox::information(this, "阅读模式", "已切换到阅读模式\n\n去除广告和干扰元素");
        });

        connect(toolbar_, &AccessibleToolbar::summarizeRequested, this, [this]() {
            QMessageBox::information(this, "页面总结",
                "📝 内容总结：\n\n"
                "这是一篇关于人工智能最新进展的报道。\n"
                "主要讨论了大语言模型在无障碍领域的应用。\n\n"
                "关键词：AI、无障碍、语音识别、图像描述");
        });

        connect(toolbar_, &AccessibleToolbar::translateRequested, this, [this]() {
            QMessageBox::information(this, "翻译", "翻译功能激活\n\n选择目标语言...");
        });

        connect(toolbar_, &AccessibleToolbar::bookmarkRequested, this, [this]() {
            QMessageBox::information(this, "书签", "✅ 已添加到书签");
        });
    }

    void loadDemoPage() {
        QString html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <style>
        body {
            font-family: "Microsoft YaHei", Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 40px 20px;
            line-height: 1.8;
            font-size: 16px;
            color: #333;
        }
        h1 {
            color: #0066FF;
            font-size: 32px;
            margin-bottom: 10px;
        }
        .meta {
            color: #666;
            font-size: 14px;
            margin-bottom: 30px;
        }
        p {
            margin-bottom: 20px;
            text-align: justify;
        }
        .highlight {
            background: #FFF3CD;
            padding: 2px 4px;
        }
    </style>
</head>
<body>
    <h1>人工智能的最新进展</h1>
    <div class="meta">科技日报 | 2024年3月15日 | 约5分钟阅读</div>

    <p>
        <span class="highlight">人工智能技术</span>正在以前所未有的速度发展，
        尤其在<strong>无障碍技术</strong>领域取得了突破性进展。
        最新的大语言模型能够理解复杂的自然语言指令，
        为视障、听障和行动不便的用户提供了全新的交互方式。
    </p>

    <p>
        语音识别技术的准确率已经超过98%，即使在嘈杂环境中也能准确识别用户的指令。
        结合自然语言处理技术，AI助手可以理解用户的真实意图，
        而不仅仅是机械地执行预设的命令。
    </p>

    <p>
        图像识别技术同样取得了重大突破。现代AI系统不仅能够识别图片中的物体，
        还能理解场景的上下文，生成自然流畅的描述。
        这对于视障用户浏览网页内容具有革命性的意义。
    </p>

    <p>
        更重要的是，这些技术正在变得越来越普及和易用。
        开发者可以轻松地将无障碍功能集成到他们的应用中，
        而用户也能以更低的成本享受到这些技术带来的便利。
    </p>

    <p>
        展望未来，AI技术将继续在无障碍领域发挥重要作用。
        我们期待看到更多创新的应用，让每个人都能平等地享受数字世界的精彩。
    </p>
</body>
</html>
        )";

        webView_->setHtml(html);
    }

    void simulateReading() {
        totalParagraphs_ = 12;
        currentParagraph_ = 1;
        toolbar_->setReadingProgress(currentParagraph_, totalParagraphs_);
        toolbar_->setReadingSpeed(1.0);

        // 模拟朗读进度
        auto *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this, timer]() {
            if (currentParagraph_ < totalParagraphs_) {
                currentParagraph_++;
                toolbar_->setReadingProgress(currentParagraph_, totalParagraphs_);
            } else {
                timer->stop();
                toolbar_->setReadingState(false);
                toolbar_->hideReadingControls();
            }
        });
        timer->start(2000); // 每2秒读一段
    }

    AccessibleToolbar *toolbar_;
    QWebEngineView *webView_;
    int currentParagraph_ = 0;
    int totalParagraphs_ = 0;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用信息
    app.setApplicationName("IntelNet Browser");
    app.setOrganizationName("IntelNet");
    app.setApplicationVersion("0.1.0");

    // 启用高 DPI 支持
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    DemoWindow window;
    window.show();

    return app.exec();
}

#include "demo_main.moc"
