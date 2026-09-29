// 浏览器组件实现
#include "browser_widget.h"
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QVariant>

namespace IntelNet {

// 注入到页面的图片选择脚本（点击图片 / ESC 取消）
static const char *kPickScript = R"JS(
(function () {
  window.__INTELNET_PICK__ = null;
  if (window.__intelnetCleanup) { try { window.__intelnetCleanup(); } catch (e) {} }

  var oldStyle = document.getElementById('__intelnet_style__');
  if (oldStyle) oldStyle.remove();
  var style = document.createElement('style');
  style.id = '__intelnet_style__';
  style.textContent = 'img:hover{outline:3px solid #667eea !important;cursor:crosshair !important;filter:brightness(1.08) !important;}';
  (document.head || document.documentElement).appendChild(style);

  var oldBanner = document.getElementById('__intelnet_banner__');
  if (oldBanner) oldBanner.remove();
  var banner = document.createElement('div');
  banner.id = '__intelnet_banner__';
  banner.textContent = '点击任意图片进行分析 · 按 ESC 取消';
  banner.style.cssText = 'position:fixed;top:12px;left:50%;transform:translateX(-50%);z-index:2147483647;background:linear-gradient(135deg,#667eea,#764ba2);color:#fff;padding:10px 22px;border-radius:24px;font-size:14px;font-family:sans-serif;box-shadow:0 4px 16px rgba(0,0,0,.3);pointer-events:none;';
  (document.body || document.documentElement).appendChild(banner);

  function cleanup() {
    document.removeEventListener('click', onClick, true);
    document.removeEventListener('keydown', onKey, true);
    var b = document.getElementById('__intelnet_banner__'); if (b) b.remove();
    var s = document.getElementById('__intelnet_style__'); if (s) s.remove();
    window.__intelnetCleanup = null;
  }

  function finish(value) {
    window.__INTELNET_PICK__ = value;
    cleanup();
  }

  function onClick(e) {
    var t = e.target;
    while (t && t.tagName !== 'IMG') t = t.parentElement;
    if (!t) return;
    e.preventDefault();
    e.stopPropagation();
    var data = null;
    try {
      var c = document.createElement('canvas');
      c.width = t.naturalWidth || t.width;
      c.height = t.naturalHeight || t.height;
      c.getContext('2d').drawImage(t, 0, 0);
      data = c.toDataURL('image/png');
    } catch (err) {
      data = t.currentSrc || t.src || null;
    }
    if (!data) data = t.currentSrc || t.src || null;
    if (data) finish(data);
  }

  function onKey(e) {
    if (e.key === 'Escape') {
      e.preventDefault();
      finish('__INTELNET_CANCEL__');
    }
  }

  document.addEventListener('click', onClick, true);
  document.addEventListener('keydown', onKey, true);
  window.__intelnetCleanup = cleanup;
})();
)JS";

BrowserWidget::BrowserWidget(QWidget *parent)
    : QWebEngineView(parent)
    , pickTimer_(new QTimer(this))
    , pickElapsedMs_(0)
{
    setupPage();

    // 连接信号
    connect(this, &QWebEngineView::urlChanged, this, &BrowserWidget::urlChanged);
    connect(this, &QWebEngineView::loadFinished, this, &BrowserWidget::loadFinished);
    connect(this, &QWebEngineView::loadProgress, this, &BrowserWidget::loadProgress);

    pickTimer_->setInterval(300);
    connect(pickTimer_, &QTimer::timeout, this, [this]() {
        pickElapsedMs_ += pickTimer_->interval();
        if (pickElapsedMs_ > 180000) { // 3 分钟超时
            pickTimer_->stop();
            auto cb = pickCallback_;
            pickCallback_ = nullptr;
            if (cb) cb(QString());
            return;
        }
        page()->runJavaScript("(window.__INTELNET_PICK__ || '')", [this](const QVariant &v) {
            const QString value = v.toString();
            if (value.isEmpty()) return;
            pickTimer_->stop();
            page()->runJavaScript("window.__INTELNET_PICK__ = null;");
            auto cb = pickCallback_;
            pickCallback_ = nullptr;
            if (cb) {
                cb(value == "__INTELNET_CANCEL__" ? QString() : value);
            }
        });
    });

    // 加载默认主页
    load(QUrl("https://www.baidu.com"));
}

BrowserWidget::~BrowserWidget() {
}

void BrowserWidget::setupPage() {
    // 配置 WebEngine 设置
    QWebEngineProfile *profile = QWebEngineProfile::defaultProfile();

    // 设置 User Agent
    profile->setHttpUserAgent(
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 IntelNet/1.0"
    );

    // 启用必要的特性
    QWebEngineSettings *settings = profile->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::AutoLoadImages, true);

    // 无障碍支持
    settings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);
    settings->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, false);
}

void BrowserWidget::load(const QUrl &url) {
    QWebEngineView::load(url);
}

void BrowserWidget::back() {
    QWebEngineView::back();
}

void BrowserWidget::forward() {
    QWebEngineView::forward();
}

void BrowserWidget::reload() {
    QWebEngineView::reload();
}

void BrowserWidget::requestPageText(std::function<void(const QString&)> callback) {
    page()->toPlainText([callback](const QString &text) {
        if (callback) {
            callback(text);
        }
    });
}

// 从 DOM 中提取结构化页面源码（在页面上下文执行，直接读取真实源码）
static const char *kPageSourceScript = R"JS(
(function () {
  function clean(s) { return (s || '').replace(/\s+/g, ' ').trim(); }
  var parts = [];

  parts.push('【URL】' + location.href);
  parts.push('【标题】' + clean(document.title));

  var md = document.querySelector('meta[name="description"], meta[property="og:description"]');
  if (md && md.content) parts.push('【描述】' + clean(md.content));

  var kw = document.querySelector('meta[name="keywords"]');
  if (kw && kw.content) parts.push('【关键词】' + clean(kw.content));

  var heads = document.querySelectorAll('h1,h2,h3');
  if (heads.length) {
    var hs = [];
    for (var i = 0; i < heads.length && i < 50; i++) {
      var t = clean(heads[i].innerText);
      if (t) hs.push(heads[i].tagName.toLowerCase() + ': ' + t);
    }
    if (hs.length) parts.push('【标题层级】\n' + hs.join('\n'));
  }

  var main = document.querySelector('article') || document.querySelector('main') || document.body;
  var text = clean(main ? main.innerText : '');
  parts.push('【正文】\n' + text.slice(0, 15000));

  var imgs = document.querySelectorAll('img');
  if (imgs.length) {
    var is = [];
    for (var j = 0; j < imgs.length && j < 30; j++) {
      var alt = clean(imgs[j].alt);
      var src = imgs[j].currentSrc || imgs[j].src || '';
      if (alt || src) is.push('- ' + (alt || '(无描述)') + ' | ' + src);
    }
    if (is.length) parts.push('【图片】\n' + is.join('\n'));
  }

  var links = document.querySelectorAll('a[href]');
  if (links.length) {
    var ls = [];
    for (var k = 0; k < links.length && k < 40; k++) {
      var lt = clean(links[k].innerText);
      if (lt && lt.length < 60) ls.push('- ' + lt + ' -> ' + links[k].href);
    }
    if (ls.length) parts.push('【链接】\n' + ls.join('\n'));
  }

  return parts.join('\n\n');
})();
)JS";

void BrowserWidget::requestPageSource(std::function<void(const QString&)> callback) {
    page()->runJavaScript(QString::fromUtf8(kPageSourceScript), [callback](const QVariant &v) {
        if (callback) {
            callback(v.toString());
        }
    });
}

void BrowserWidget::requestImageUrls(std::function<void(const QStringList&)> callback) {
    static const char *kScript =
        "(function(){"
        "var out=[];var imgs=document.images;"
        "for(var i=0;i<imgs.length;i++){var s=imgs[i].currentSrc||imgs[i].src;if(s)out.push(s);}"
        "return JSON.stringify(out);"
        "})();";

    page()->runJavaScript(kScript, [callback](const QVariant &v) {
        QStringList urls;
        const QJsonDocument doc = QJsonDocument::fromJson(v.toString().toUtf8());
        if (doc.isArray()) {
            for (const QJsonValue &val : doc.array()) {
                const QString u = val.toString();
                if (!u.isEmpty()) {
                    urls << u;
                }
            }
        }
        if (callback) {
            callback(urls);
        }
    });
}

void BrowserWidget::pickImage(std::function<void(const QString&)> callback) {
    pickCallback_ = callback;
    pickElapsedMs_ = 0;
    injectPickScript();
    pickTimer_->start();
}

void BrowserWidget::cancelPickImage() {
    page()->runJavaScript("window.__INTELNET_PICK__ = '__INTELNET_CANCEL__';");
}

void BrowserWidget::injectPickScript() {
    page()->runJavaScript(QString::fromUtf8(kPickScript));
}

} // namespace IntelNet
