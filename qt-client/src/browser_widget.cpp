// 浏览器组件实现
#include "browser_widget.h"
#include "browser_page.h"
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineHistory>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>
#include <QBuffer>

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

// 监测页面内弹窗并标出可用的关闭按钮。
static const char *kPopupObserverScript = R"JS(
(function () {
  if (window.__intelnetPopupCleanup) window.__intelnetPopupCleanup();
  window.__INTELNET_POPUP_EVENT__ = '';

  var seen = new WeakSet();
  var style = document.getElementById('__intelnet_popup_style__');
  if (!style) {
    style = document.createElement('style');
    style.id = '__intelnet_popup_style__';
    style.textContent = '@keyframes intelnetPopupPulse{50%{outline-color:#ff0000;box-shadow:0 0 0 6px rgba(255,0,0,.3)}}' +
      '.__intelnet_popup_close__{outline:3px solid #ff0000 !important;outline-offset:2px !important;' +
      'box-shadow:0 0 0 3px rgba(255,0,0,.45) !important;animation:intelnetPopupPulse 1s ease-in-out infinite !important;}';
    (document.head || document.documentElement).appendChild(style);
  }

  function isPopup(el) {
    if (!el || el.nodeType !== 1) return false;
    var css = getComputedStyle(el);
    var rect = el.getBoundingClientRect();
    var z = parseInt(css.zIndex, 10);
    return css.position === 'fixed' && rect.width > innerWidth * 0.5 &&
      rect.height > innerHeight * 0.5 && !isNaN(z) && z >= 1000;
  }

  function visible(el) {
    var rect = el.getBoundingClientRect();
    var css = getComputedStyle(el);
    return rect.width > 0 && rect.height > 0 && css.visibility !== 'hidden' &&
      css.display !== 'none';
  }

  function findDialogBox(popup) {
    var rect = popup.getBoundingClientRect();
    var full = rect.width >= innerWidth * 0.95 && rect.height >= innerHeight * 0.95;
    if (!full) return popup;
    var best = null, bestArea = 0;
    var els = popup.querySelectorAll('*');
    for (var i = 0; i < els.length; i++) {
      var r = els[i].getBoundingClientRect();
      if (r.width < 80 || r.height < 60) continue;
      if (r.width >= innerWidth * 0.95 && r.height >= innerHeight * 0.95) continue;
      var area = r.width * r.height;
      if (area > bestArea) { bestArea = area; best = els[i]; }
    }
    return best || popup;
  }

  function findCloseButton(popup) {
    var box = findDialogBox(popup);
    var boxRect = box.getBoundingClientRect();
    var candidates = popup.querySelectorAll('button,[role="button"],[aria-label]');
    var i;
    for (i = 0; i < candidates.length; i++) {
      var label = (candidates[i].getAttribute('aria-label') || '').toLowerCase();
      if (visible(candidates[i]) && (label.indexOf('关闭') >= 0 || label.indexOf('close') >= 0)) {
        return candidates[i];
      }
    }

    candidates = popup.querySelectorAll('button,[role="button"],a,span,div');
    for (i = 0; i < candidates.length; i++) {
      var text = (candidates[i].textContent || '').trim();
      if (visible(candidates[i]) && (text === '×' || text === '✕' || text === '关闭')) {
        return candidates[i];
      }
    }

    candidates = popup.querySelectorAll('button,[role="button"]');
    for (i = 0; i < candidates.length; i++) {
      var rect = candidates[i].getBoundingClientRect();
      if (visible(candidates[i]) && rect.left >= boxRect.left + boxRect.width * 0.85 &&
          rect.top <= boxRect.top + boxRect.height * 0.15) {
        return candidates[i];
      }
    }
    return null;
  }

  function inspect(popup) {
    if (!isPopup(popup) || seen.has(popup)) return;
    var closeButton = findCloseButton(popup);
    if (!closeButton) return;
    seen.add(popup);
    closeButton.classList.add('__intelnet_popup_close__');
    window.__INTELNET_POPUP_EVENT__ = 'detected';
  }

  function scan(root) {
    if (!root || root.nodeType !== 1) return;
    inspect(root);
    var descendants = root.querySelectorAll('*');
    for (var i = 0; i < descendants.length; i++) inspect(descendants[i]);
    var parent = root.parentElement;
    while (parent) {
      inspect(parent);
      parent = parent.parentElement;
    }
  }

  scan(document.body || document.documentElement);
  var observer = new MutationObserver(function (mutations) {
    mutations.forEach(function (mutation) {
      for (var i = 0; i < mutation.addedNodes.length; i++) scan(mutation.addedNodes[i]);
    });
  });
  observer.observe(document.documentElement, {childList: true, subtree: true});
  window.__intelnetPopupCleanup = function () {
    observer.disconnect();
    var marked = document.querySelectorAll('.__intelnet_popup_close__');
    for (var i = 0; i < marked.length; i++) marked[i].classList.remove('__intelnet_popup_close__');
    if (style.parentNode) style.parentNode.removeChild(style);
    window.__intelnetPopupCleanup = null;
  };
})();
)JS";

static const char *kFormGuardScript = R"JS(
(function () {
  window.__INTELNET_FORM_EVENT__ = '';
  window.__INTELNET_FORM_ACTIVE__ = false;
  window.__INTELNET_FORM_CONFIRM__ = false;
  window.__INTELNET_FORM_PENDING__ = null;

  function labelFor(el) {
    var labels = document.querySelectorAll('label');
    for (var i = 0; i < labels.length; i++) {
      if (el.id && labels[i].htmlFor === el.id) return labels[i].innerText.trim();
    }
    var wrapped = el.closest && el.closest('label');
    if (wrapped) return wrapped.innerText.trim();
    var ids = (el.getAttribute('aria-labelledby') || '').trim().split(/\s+/);
    for (var j = 0; j < ids.length; j++) {
      var node = ids[j] && document.getElementById(ids[j]);
      if (node && node.innerText.trim()) return node.innerText.trim();
    }
    return (el.getAttribute('placeholder') || el.name || el.id || '未命名字段').trim();
  }

  function controls(root) {
    return root.querySelectorAll('input,select,textarea');
  }

  function describeRoot(root) {
    var fields = [], nodes = controls(root);
    for (var i = 0; i < nodes.length; i++) {
      var el = nodes[i], type = (el.type || el.tagName).toLowerCase();
      if (type === 'hidden' || el.disabled) continue;
      var options = [];
      if (type === 'select-one' || type === 'select-multiple') {
        for (var j = 0; j < el.options.length; j++) options.push(el.options[j].text.trim());
      }
      fields.push({fieldIndex:i,type:type,label:labelFor(el),required:!!(el.required || el.getAttribute('aria-required') === 'true'),placeholder:el.placeholder || '',options:options});
    }
    return fields;
  }

  function valuePayload(root, formIndex, submitButton) {
    var fields = [], nodes = controls(root);
    for (var i = 0; i < nodes.length; i++) {
      var el = nodes[i], type = (el.type || el.tagName).toLowerCase();
      if (type === 'hidden' || el.disabled) continue;
      fields.push({fieldIndex:i,type:type,label:labelFor(el),required:!!(el.required || el.getAttribute('aria-required') === 'true'),value:type === 'password' ? '' : (el.value || '')});
    }
    return {formIndex:formIndex,fields:fields,submitButton:submitButton || null};
  }

  function intercept(root, formIndex, submitButton) {
    if (root.__intelnetGuarded) return;
    root.__intelnetGuarded = true;
    root.addEventListener('submit', function (e) {
      if (!window.__INTELNET_FORM_ACTIVE__) return;
      if (window.__INTELNET_FORM_CONFIRM__) {
        window.__INTELNET_FORM_CONFIRM__ = false;
        window.__INTELNET_FORM_PENDING__ = null;
        return;
      }
      e.preventDefault();
      window.__INTELNET_FORM_PENDING__ = root;
      window.__INTELNET_FORM_EVENT__ = JSON.stringify(valuePayload(root, formIndex, submitButton));
    }, true);
    if (submitButton) submitButton.addEventListener('click', function (e) {
      if (!window.__INTELNET_FORM_ACTIVE__) return;
      if (window.__INTELNET_FORM_CONFIRM__) {
        window.__INTELNET_FORM_CONFIRM__ = false;
        window.__INTELNET_FORM_PENDING__ = null;
        return;
      }
      if (e.defaultPrevented) return;
      e.preventDefault();
      window.__INTELNET_FORM_PENDING__ = root;
      window.__INTELNET_FORM_EVENT__ = JSON.stringify(valuePayload(root, formIndex, submitButton));
    }, true);
  }

  function scan() {
    var forms = document.querySelectorAll('form');
    for (var i = 0; i < forms.length; i++) intercept(forms[i], i, null);
    var submits = document.querySelectorAll('button[type="submit"],input[type="submit"]');
    for (var j = 0; j < submits.length; j++) {
      if (!submits[j].form) intercept(submits[j].parentElement, -1, submits[j]);
    }
  }

  document.addEventListener('keydown', function (e) {
    if (e.key !== 'Enter' || !window.__INTELNET_FORM_PENDING__) return;
    var root = window.__INTELNET_FORM_PENDING__;
    window.__INTELNET_FORM_CONFIRM__ = true;
    e.preventDefault();
    if (root.requestSubmit) root.requestSubmit();
    else if (root.__intelnetSubmitButton) root.__intelnetSubmitButton.click();
  }, true);
  scan();
  if (window.__intelnetFormObserver) window.__intelnetFormObserver.disconnect();
  window.__intelnetFormObserver = new MutationObserver(scan);
  window.__intelnetFormObserver.observe(document.documentElement, {childList:true,subtree:true});
})();
)JS";

BrowserWidget::BrowserWidget(QWidget *parent)
    : QWebEngineView(parent)
    , pickTimer_(new QTimer(this))
    , popupTimer_(new QTimer(this))
    , formTimer_(new QTimer(this))
    , pickElapsedMs_(0)
{
    setupPage();
    auto *intelNetPage = new IntelNetPage(this);
    setPage(intelNetPage);

    // 连接信号
    connect(this, &QWebEngineView::urlChanged, this, &BrowserWidget::urlChanged);
    connect(this, &QWebEngineView::loadFinished, this, &BrowserWidget::loadFinished);
    connect(this, &QWebEngineView::loadProgress, this, &BrowserWidget::loadProgress);
    connect(intelNetPage, &IntelNetPage::redirectChainBlocked,
            this, &BrowserWidget::redirectChainBlocked);
    connect(intelNetPage, &IntelNetPage::warningActionRequested,
            this, &BrowserWidget::warningActionRequested);
    connect(intelNetPage, &IntelNetPage::repeatedAlertBlocked,
            this, &BrowserWidget::repeatedAlertBlocked);
    connect(intelNetPage, &IntelNetPage::openUrlRequested,
            this, [this](const QUrl &url) { load(url); });
    connect(this, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (ok) {
            installPopupObserver();
            installFormGuards();
        }
    });

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

    popupTimer_->setInterval(250);
    connect(popupTimer_, &QTimer::timeout, this, [this]() {
        page()->runJavaScript("window.__INTELNET_POPUP_EVENT__ || ''",
            [this](const QVariant &v) {
                if (v.toString().isEmpty()) return;
                page()->runJavaScript("window.__INTELNET_POPUP_EVENT__ = '';");
                emit popupCloseButtonDetected();
            });
    });

    formTimer_->setInterval(250);
    connect(formTimer_, &QTimer::timeout, this, [this]() {
        page()->runJavaScript("window.__INTELNET_FORM_EVENT__ || ''",
            [this](const QVariant &v) {
                const QString event = v.toString();
                if (event.isEmpty()) return;
                page()->runJavaScript("window.__INTELNET_FORM_EVENT__ = '';");
                emit formSubmitIntercepted(event);
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

void BrowserWidget::installPopupObserver() {
    page()->runJavaScript(QString::fromUtf8(kPopupObserverScript));
    popupTimer_->start();
}

void BrowserWidget::installFormGuards() {
    page()->runJavaScript(QString::fromUtf8(kFormGuardScript));
    formTimer_->start();
}

void BrowserWidget::load(const QUrl &url) {
    QWebEngineView::load(url);
}

void BrowserWidget::back() {
    if (page()->history()->canGoBack()) {
        page()->history()->back();
    }
}

void BrowserWidget::forward() {
    if (page()->history()->canGoForward()) {
        page()->history()->forward();
    }
}

void BrowserWidget::reload() {
    QWebEngineView::reload();
}

void BrowserWidget::allowNavigationOnce(const QUrl &url) {
    if (auto *intelNetPage = qobject_cast<IntelNetPage *>(page())) {
        intelNetPage->allowNavigationOnce(url);
    }
}

void BrowserWidget::resetNavigationHistory() {
    if (auto *intelNetPage = qobject_cast<IntelNetPage *>(page())) {
        intelNetPage->resetNavigationHistory();
    }
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

void BrowserWidget::requestFormStructure(std::function<void(const QString&)> callback) {
    static const char *kScript = R"JS(
(function () {
  window.__INTELNET_FORM_ACTIVE__ = true;
  function labelFor(el) {
    var labels = document.querySelectorAll('label');
    for (var i = 0; i < labels.length; i++) {
      if (el.id && labels[i].htmlFor === el.id) return labels[i].innerText.trim();
    }
    var wrapped = el.closest && el.closest('label');
    if (wrapped) return wrapped.innerText.trim();
    var ids = (el.getAttribute('aria-labelledby') || '').trim().split(/\s+/);
    for (var j = 0; j < ids.length; j++) {
      var node = ids[j] && document.getElementById(ids[j]);
      if (node && node.innerText.trim()) return node.innerText.trim();
    }
    return (el.getAttribute('placeholder') || el.name || el.id || '未命名字段').trim();
  }
  function read(root) {
    var fields = [], nodes = root.querySelectorAll('input,select,textarea');
    for (var i = 0; i < nodes.length; i++) {
      var el = nodes[i], type = (el.type || el.tagName).toLowerCase();
      if (type === 'hidden' || el.disabled) continue;
      var options = [];
      if (type === 'select-one' || type === 'select-multiple') {
        for (var j = 0; j < el.options.length; j++) options.push(el.options[j].text.trim());
      }
      fields.push({type:type,label:labelFor(el),required:!!(el.required || el.getAttribute('aria-required') === 'true'),placeholder:el.placeholder || '',options:options});
    }
    return fields;
  }
  var forms = document.querySelectorAll('form'), out = [];
  for (var i = 0; i < forms.length; i++) out.push({formIndex:i,fields:read(forms[i])});
  if (!out.length) {
    var submit = document.querySelector('button[type="submit"],input[type="submit"]');
    if (submit && submit.parentElement) out.push({formIndex:-1,fields:read(submit.parentElement)});
  }
  return JSON.stringify(out);
})();
)JS";
    page()->runJavaScript(QString::fromUtf8(kScript), [callback](const QVariant &v) {
        if (callback) callback(v.toString());
    });
}

void BrowserWidget::armFormConfirmation() {
    page()->runJavaScript("window.__INTELNET_FORM_CONFIRM__ = false;");
}

void BrowserWidget::cancelFormConfirmation() {
    page()->runJavaScript("window.__INTELNET_FORM_PENDING__ = null;window.__INTELNET_FORM_CONFIRM__ = false;");
}

void BrowserWidget::focusFormField(int formIndex, int fieldIndex) {
    const QString script = QString(
        "(function(){var fs=document.querySelectorAll('form');var r=%1>=0&&fs[%1]?fs[%1]:document;"
        "var c=r.querySelectorAll('input,select,textarea');if(c[%2])c[%2].focus();})();")
        .arg(formIndex).arg(fieldIndex);
    page()->runJavaScript(script);
}

void BrowserWidget::requestHeadingOutline(
    std::function<void(const QList<HeadingEntry>&)> callback) {
    static const char *kScript = R"JS(
(function () {
  var out = [];
  var heads = document.querySelectorAll('h1,h2,h3,h4,h5,h6');
  for (var i = 0; i < heads.length; i++) {
    var h = heads[i];
    var text = (h.innerText || '').replace(/\s+/g, ' ').trim();
    if (!text) continue;
    var p = h.parentElement, chrome = false;
    while (p && p !== document.body) {
      var tag = p.tagName.toLowerCase();
      if (tag === 'nav' || tag === 'header' || tag === 'footer' || tag === 'aside') {
        chrome = true;
        break;
      }
      p = p.parentElement;
    }
    if (chrome) continue;
    if (!h.id) h.id = '__intelnet_h_' + i;
    out.push({level: parseInt(h.tagName[1]), text: text, id: h.id});
  }
  return JSON.stringify(out);
})();
)JS";

    page()->runJavaScript(QString::fromUtf8(kScript),
        [callback](const QVariant &v) {
            QList<HeadingEntry> headings;
            const QJsonDocument doc = QJsonDocument::fromJson(v.toString().toUtf8());
            if (doc.isArray()) {
                for (const QJsonValue &value : doc.array()) {
                    if (!value.isObject()) continue;
                    const QJsonObject object = value.toObject();
                    const QString text = object.value("text").toString().trimmed();
                    const QString id = object.value("id").toString();
                    const int level = object.value("level").toInt(0);
                    if (!text.isEmpty() && !id.isEmpty() && level >= 1 && level <= 6) {
                        headings.append({level, text, id});
                    }
                }
            }
            if (callback) callback(headings);
        });
}

void BrowserWidget::scrollToHeading(const QString &id) {
    QJsonArray values;
    values.append(id);
    const QString jsonArray = QString::fromUtf8(
        QJsonDocument(values).toJson(QJsonDocument::Compact));
    const QString jsonLiteral = jsonArray.mid(1, jsonArray.size() - 2);
    const QString script = QString(
        "(function(){var h=document.getElementById(%1);"
        "if(h)h.scrollIntoView({block:'start',behavior:'smooth'});})();")
        .arg(jsonLiteral);
    page()->runJavaScript(script);
}

void BrowserWidget::requestUnnamedButtons(
    std::function<void(const QList<UnnamedButton>&)> callback) {
    static const char *kScript = R"JS(
(function () {
  var out = [];
  var nodes = document.querySelectorAll(
    'button,input[type="button"],input[type="submit"],[role="button"]');
  for (var i = 0; i < nodes.length; i++) {
    var el = nodes[i];
    var text = (el.innerText || '').replace(/\s+/g, ' ').trim();
    var aria = (el.getAttribute('aria-label') || '').trim();
    var title = (el.getAttribute('title') || '').trim();
    var value = (el.value || '').trim();
    if (text || aria || title || value) continue;

    var rect = el.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) continue;
    if (!el.id) el.id = '__intelnet_button_' + i;
    out.push({id: el.id, x: rect.x, y: rect.y, width: rect.width, height: rect.height});
  }
  return JSON.stringify(out);
})();
)JS";

    page()->runJavaScript(QString::fromUtf8(kScript),
        [callback](const QVariant &v) {
            QList<UnnamedButton> buttons;
            const QJsonDocument doc = QJsonDocument::fromJson(v.toString().toUtf8());
            if (doc.isArray()) {
                for (const QJsonValue &value : doc.array()) {
                    if (!value.isObject()) continue;
                    const QJsonObject object = value.toObject();
                    const QString id = object.value("id").toString();
                    const double x = object.value("x").toDouble(-1);
                    const double y = object.value("y").toDouble(-1);
                    const double width = object.value("width").toDouble(0);
                    const double height = object.value("height").toDouble(0);
                    if (id.isEmpty() || width <= 0 || height <= 0) continue;
                    buttons.append({id, QRectF(x, y, width, height)});
                }
            }
            if (callback) callback(buttons);
        });
}

void BrowserWidget::captureElement(const QString &id,
                                   std::function<void(const QImage&)> callback) {
    QJsonArray values;
    values.append(id);
    const QString jsonArray = QString::fromUtf8(
        QJsonDocument(values).toJson(QJsonDocument::Compact));
    const QString jsonLiteral = jsonArray.mid(1, jsonArray.size() - 2);
    const QString script = QString(
        "(function(){var e=document.getElementById(%1);"
        "if(!e)return null;e.scrollIntoView({block:'center'});"
        "var r=e.getBoundingClientRect();"
        "return JSON.stringify({x:r.x,y:r.y,width:r.width,height:r.height});})();")
        .arg(jsonLiteral);
    page()->runJavaScript(script, [this, callback](const QVariant &v) {
        const QJsonDocument doc = QJsonDocument::fromJson(v.toString().toUtf8());
        if (!doc.isObject()) {
            if (callback) callback(QImage());
            return;
        }
        const QJsonObject object = doc.object();
        const QRectF rect(object.value("x").toDouble(), object.value("y").toDouble(),
                          object.value("width").toDouble(), object.value("height").toDouble());
        QTimer::singleShot(120, this, [this, rect, callback]() {
            const QPixmap shot = grab();
            const qreal dpr = shot.devicePixelRatio();
            const QRect crop(qRound(rect.x() * dpr), qRound(rect.y() * dpr),
                             qRound(rect.width() * dpr), qRound(rect.height() * dpr));
            const QRect bounded = crop.intersected(QRect(QPoint(0, 0), shot.size()));
            if (callback) callback(bounded.isEmpty() ? QImage() : shot.copy(bounded).toImage());
        });
    });
}

void BrowserWidget::setButtonAriaLabel(const QString &id, const QString &label) {
    QJsonArray values;
    values.append(id);
    values.append(label);
    const QString json = QString::fromUtf8(
        QJsonDocument(values).toJson(QJsonDocument::Compact));
    page()->runJavaScript(QString(
        "(function(){var a=%1;var e=document.getElementById(a[0]);"
        "if(e)e.setAttribute('aria-label',a[1]);})();").arg(json));
}

void BrowserWidget::requestCaptchaCandidate(
    std::function<void(const CaptchaCandidate&)> callback) {
    static const char *kScript = R"JS(
(function () {
  var best = null, bestScore = 0;
  var imgs = document.querySelectorAll('img');
  for (var i = 0; i < imgs.length; i++) {
    var img = imgs[i], rect = img.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) continue;
    var score = 0, p = img.parentElement;
    while (p && p !== document.body) {
      if (p.tagName.toLowerCase() === 'form') { score += 100; break; }
      p = p.parentElement;
    }
    p = img.parentElement;
    for (var j = 0; p && j < 3; j++, p = p.parentElement) {
      if (p.querySelector('input[type="password"]')) { score += 50; break; }
    }
    var meta = ((img.src || '') + ' ' + (img.alt || '')).toLowerCase();
    if (/captcha|verify|yzm|验证码/.test(meta)) score += 25;
    if (!score) continue;
    if (!img.id) img.id = '__intelnet_captcha_' + i;
    if (!best || score > bestScore) {
      best = {id: img.id, src: img.currentSrc || img.src || ''};
      bestScore = score;
    }
  }
  return JSON.stringify(best || {});
})();
)JS";
    page()->runJavaScript(QString::fromUtf8(kScript), [callback](const QVariant &v) {
        CaptchaCandidate candidate;
        const QJsonDocument doc = QJsonDocument::fromJson(v.toString().toUtf8());
        if (doc.isObject()) {
            candidate.id = doc.object().value("id").toString();
            candidate.src = doc.object().value("src").toString();
        }
        if (callback) callback(candidate);
    });
}

void BrowserWidget::playAudioCaptcha(std::function<void(bool)> callback) {
    page()->runJavaScript(R"JS(
(function () {
  window.__INTELNET_AUDIO_RESULT__ = 'pending';
  var found = false;
  var nodes = document.querySelectorAll('button,a,[role="button"]');
  for (var i = 0; i < nodes.length; i++) {
    var text = (nodes[i].innerText || nodes[i].getAttribute('aria-label') || '').trim();
    if (/语音验证码|音频验证码|听语音/.test(text)) { nodes[i].click(); found = true; break; }
  }
  setTimeout(function () {
    var audio = document.querySelector('audio');
    if (!audio) { window.__INTELNET_AUDIO_RESULT__ = found ? 'notfound' : 'notfound'; return; }
    audio.currentTime = 0;
    var promise = audio.play();
    if (promise && promise.then) promise.then(function () {
      window.__INTELNET_AUDIO_RESULT__ = 'playing';
    }).catch(function () { window.__INTELNET_AUDIO_RESULT__ = 'blocked'; });
    else window.__INTELNET_AUDIO_RESULT__ = 'playing';
  }, 500);
})();
)JS", [this, callback] (const QVariant &) {
        QTimer::singleShot(900, this, [this, callback]() {
            page()->runJavaScript("window.__INTELNET_AUDIO_RESULT__ || 'notfound'",
                [callback](const QVariant &v) {
                    if (callback) callback(v.toString() == "playing");
                });
        });
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

void BrowserWidget::requestImagesWithoutAlt(
    std::function<void(const QList<MissingAltImage>&)> callback) {
    static const char *kScript = R"JS(
(function () {
  var out = [];
  var imgs = document.images;
  for (var i = 0; i < imgs.length; i++) {
    if (imgs[i].hasAttribute('alt')) continue;
    var w = imgs[i].naturalWidth || 0;
    if (w > 0 && w < 48) continue;
    var src = imgs[i].currentSrc || imgs[i].src || '';
    if (src) {
      if (!imgs[i].id) imgs[i].id = '__intelnet_alt_' + i;
      out.push({idx: i, id: imgs[i].id, src: src});
    }
  }
  return JSON.stringify(out);
})();
)JS";

    page()->runJavaScript(QString::fromUtf8(kScript),
        [callback](const QVariant &v) {
            QList<MissingAltImage> images;
            const QJsonDocument doc = QJsonDocument::fromJson(v.toString().toUtf8());
            if (doc.isArray()) {
                for (const QJsonValue &value : doc.array()) {
                    if (!value.isObject()) continue;
                    const QJsonObject object = value.toObject();
                    const int index = object.value("idx").toInt(-1);
                    const QString id = object.value("id").toString();
                    const QString src = object.value("src").toString();
                    if (index < 0 || id.isEmpty() || src.isEmpty()) continue;
                    images.append({index, id, src});
                }
            }
            if (callback) callback(images);
        });
}

void BrowserWidget::setImageAlt(int index, const QString &alt) {
    if (index < 0) return;

    QJsonArray values;
    values.append(alt);
    const QString jsonArray = QString::fromUtf8(
        QJsonDocument(values).toJson(QJsonDocument::Compact));
    const QString jsonLiteral = jsonArray.mid(1, jsonArray.size() - 2);
    const QString script = QString(
        "(function(){var img=document.images[%1];if(img)img.alt=%2;})();")
        .arg(index)
        .arg(jsonLiteral);
    page()->runJavaScript(script);
}

void BrowserWidget::setImageAltById(const QString &id, const QString &alt) {
    QJsonArray values;
    values.append(id);
    values.append(alt);
    const QString json = QString::fromUtf8(
        QJsonDocument(values).toJson(QJsonDocument::Compact));
    page()->runJavaScript(QString(
        "(function(){var a=%1;var e=document.getElementById(a[0]);"
        "if(e)e.alt=a[1];})();").arg(json));
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
