#ifndef BROWSER_PAGE_H
#define BROWSER_PAGE_H

#include <QDateTime>
#include <QList>
#include <QPair>
#include <QUrl>
#include <QWebEnginePage>

namespace IntelNet {

class PopupCatcher : public QWebEnginePage {
    Q_OBJECT

public:
    explicit PopupCatcher(QWebEngineProfile *profile, QObject *parent = nullptr);

signals:
    void urlRequested(const QUrl &url);

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type,
                                 bool isMainFrame) override;
};

class IntelNetPage : public QWebEnginePage {
    Q_OBJECT

public:
    explicit IntelNetPage(QObject *parent = nullptr);

    void allowNavigationOnce(const QUrl &url);
    void resetNavigationHistory();

signals:
    void redirectChainBlocked(const QUrl &url);
    void warningActionRequested(const QString &action, const QUrl &url);
    void repeatedAlertBlocked();
    void openUrlRequested(const QUrl &url);

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type,
                                 bool isMainFrame) override;
    QWebEnginePage *createWindow(WebWindowType type) override;
    void javaScriptAlert(const QUrl &securityOrigin, const QString &msg) override;

private:
    void pruneRedirects(qint64 now);
    void prunePopupWindows(qint64 now);
    void pruneAlerts(qint64 now);

    QList<QPair<QString, qint64>> redirectHistory_;
    QList<qint64> popupWindows_;
    QList<qint64> alertTimes_;
    QUrl allowedOnce_;
};

} // namespace IntelNet

#endif // BROWSER_PAGE_H
