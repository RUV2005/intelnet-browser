#include "browser_page.h"

#include <QDebug>
#include <QUrlQuery>

namespace IntelNet {

namespace {
constexpr qint64 kRedirectWindowMs = 10'000;
constexpr qint64 kPopupWindowMs = 3'000;
constexpr qint64 kAlertWindowMs = 10'000;
}

IntelNetPage::IntelNetPage(QObject *parent)
    : QWebEnginePage(parent) {
}

void IntelNetPage::allowNavigationOnce(const QUrl &url) {
    allowedOnce_ = url;
}

void IntelNetPage::resetNavigationHistory() {
    redirectHistory_.clear();
}

void IntelNetPage::pruneRedirects(qint64 now) {
    while (!redirectHistory_.isEmpty() &&
           now - redirectHistory_.first().second > kRedirectWindowMs) {
        redirectHistory_.removeFirst();
    }
}

void IntelNetPage::prunePopupWindows(qint64 now) {
    while (!popupWindows_.isEmpty() && now - popupWindows_.first() > kPopupWindowMs) {
        popupWindows_.removeFirst();
    }
}

void IntelNetPage::pruneAlerts(qint64 now) {
    while (!alertTimes_.isEmpty() && now - alertTimes_.first() > kAlertWindowMs) {
        alertTimes_.removeFirst();
    }
}

bool IntelNetPage::acceptNavigationRequest(const QUrl &url, NavigationType type,
                                           bool isMainFrame) {
    if (url.scheme() == "intelnet") {
        const QString action = url.host();
        QUrl target;
        if (action == "continue") {
            QUrlQuery query(url);
            target = QUrl(query.queryItemValue("url", QUrl::FullyDecoded));
        }
        emit warningActionRequested(action, target);
        return false;
    }

    if (!isMainFrame) {
        return true;
    }

    if (!allowedOnce_.isEmpty() && url == allowedOnce_) {
        allowedOnce_.clear();
        resetNavigationHistory();
        return true;
    }

    const bool suspiciousNavigation = type == QWebEnginePage::NavigationTypeRedirect ||
                                      type == QWebEnginePage::NavigationTypeOther;
    if (!suspiciousNavigation) {
        resetNavigationHistory();
        return true;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    pruneRedirects(now);
    const QString host = url.host().toLower();
    if (host.isEmpty()) return true;

    redirectHistory_.append({host, now});
    if (redirectHistory_.size() >= 4) {
        qWarning() << "已拦截可疑连环跳转:" << url;
        redirectHistory_.clear();
        emit redirectChainBlocked(url);
        return false;
    }

    return true;
}

QWebEnginePage *IntelNetPage::createWindow(WebWindowType type) {
    Q_UNUSED(type);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    prunePopupWindows(now);
    popupWindows_.append(now);
    if (popupWindows_.size() >= 3) {
        qWarning() << "已拦截页面发起的第" << popupWindows_.size() << "个新窗口请求";
        return nullptr;
    }

    // 当前客户端没有标签页容器，前两个请求在当前页面承载，保留 OAuth 等正常流程。
    return this;
}

void IntelNetPage::javaScriptAlert(const QUrl &securityOrigin, const QString &msg) {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    pruneAlerts(now);
    alertTimes_.append(now);
    if (alertTimes_.size() >= 3) {
        qWarning() << "已屏蔽重复 JavaScript alert:" << securityOrigin << msg;
        emit repeatedAlertBlocked();
        return;
    }

    QWebEnginePage::javaScriptAlert(securityOrigin, msg);
}

} // namespace IntelNet
