#ifndef MODEL_DOWNLOADER_H
#define MODEL_DOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QVector>

namespace IntelNet {

class ModelDownloader : public QObject {
    Q_OBJECT

public:
    explicit ModelDownloader(QObject *parent = nullptr);

    static bool modelsPresent();
    void start();
    void cancel();

signals:
    void progress(qint64 downloaded, qint64 total, const QString &fileName);
    void completed();
    void failed(const QString &message);
    void cancelled();

private slots:
    void onReadyRead();
    void onDownloadProgress(qint64 received, qint64 total);
    void onFinished();

private:
    struct FileSpec {
        QString relativePath;
        QUrl url;
    };

    static QVector<FileSpec> fileSpecs();
    static QString resourcesRoot();
    void downloadNext();
    void finishCurrent(bool success);

    QNetworkAccessManager manager_;
    QNetworkReply *reply_;
    QFile output_;
    QVector<FileSpec> files_;
    int currentIndex_;
    qint64 completedBytes_;
    qint64 currentTotal_;
    qint64 currentReceived_;
    bool cancelledByUser_;
};

} // namespace IntelNet

#endif // MODEL_DOWNLOADER_H
