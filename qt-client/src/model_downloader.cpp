#include "model_downloader.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace IntelNet {

ModelDownloader::ModelDownloader(QObject *parent)
    : QObject(parent)
    , reply_(nullptr)
    , currentIndex_(0)
    , completedBytes_(0)
    , currentTotal_(0)
    , currentReceived_(0)
    , cancelledByUser_(false) {
}

QString ModelDownloader::resourcesRoot() {
    return QDir(QCoreApplication::applicationDirPath()).filePath("resources");
}

QVector<ModelDownloader::FileSpec> ModelDownloader::fileSpecs() {
    const QString base = "https://www.modelscope.cn/models/";
    return {
        {"models/paddleocr-vl/PaddleOCR-VL-1.6-GGUF.gguf",
         QUrl(base + "PaddlePaddle/PaddleOCR-VL-1.6-GGUF/resolve/master/PaddleOCR-VL-1.6-GGUF.gguf")},
        {"models/paddleocr-vl/PaddleOCR-VL-1.6-GGUF-mmproj.gguf",
         QUrl(base + "PaddlePaddle/PaddleOCR-VL-1.6-GGUF/resolve/master/PaddleOCR-VL-1.6-GGUF-mmproj.gguf")},
        {"models/qwen2vl/model.gguf",
         QUrl(base + "danmo6321/intelnet-Qwen2-VL-2B-GGUF/resolve/master/model.gguf")},
        {"models/qwen2vl/mmproj.gguf",
         QUrl(base + "danmo6321/intelnet-Qwen2-VL-2B-GGUF/resolve/master/mmproj.gguf")},
        {"piper/zh_CN-chaowei-medium.onnx",
         QUrl(base + "danmo6321/intelnet-piper-voices/resolve/master/zh_CN-chaowei-medium.onnx")},
        {"piper/zh_CN-chaowei-medium.onnx.json",
         QUrl(base + "danmo6321/intelnet-piper-voices/resolve/master/zh_CN-chaowei-medium.onnx.json")}
    };
}

bool ModelDownloader::modelsPresent() {
    const QDir root(resourcesRoot());
    for (const auto &spec : fileSpecs()) {
        const QFileInfo info(root.filePath(spec.relativePath));
        if (!info.exists() || info.size() <= 0) return false;
    }
    return true;
}

void ModelDownloader::start() {
    cancel();
    files_ = fileSpecs();
    currentIndex_ = 0;
    completedBytes_ = 0;
    cancelledByUser_ = false;
    for (const auto &spec : files_) {
        const QFileInfo info(QDir(resourcesRoot()).filePath(spec.relativePath));
        if (info.exists() && info.size() > 0) completedBytes_ += info.size();
    }
    downloadNext();
}

void ModelDownloader::cancel() {
    cancelledByUser_ = true;
    if (reply_) reply_->abort();
}

void ModelDownloader::downloadNext() {
    while (currentIndex_ < files_.size()) {
        const auto &spec = files_.at(currentIndex_);
        const QString destination = QDir(resourcesRoot()).filePath(spec.relativePath);
        const QFileInfo info(destination);
        if (info.exists() && info.size() > 0) {
            ++currentIndex_;
            continue;
        }

        QDir().mkpath(info.absolutePath());
        output_.setFileName(destination + ".part");
        if (!output_.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            emit failed(QString("无法写入模型文件：%1").arg(destination));
            return;
        }

        currentReceived_ = 0;
        currentTotal_ = 0;
        reply_ = manager_.get(QNetworkRequest(spec.url));
        connect(reply_, &QNetworkReply::readyRead, this, &ModelDownloader::onReadyRead);
        connect(reply_, &QNetworkReply::downloadProgress, this, &ModelDownloader::onDownloadProgress);
        connect(reply_, &QNetworkReply::finished, this, &ModelDownloader::onFinished);
        return;
    }
    emit progress(completedBytes_, completedBytes_, QString());
    emit completed();
}

void ModelDownloader::onReadyRead() {
    if (reply_ && output_.isOpen()) output_.write(reply_->readAll());
}

void ModelDownloader::onDownloadProgress(qint64 received, qint64 total) {
    currentReceived_ = received;
    currentTotal_ = total;
    emit progress(completedBytes_ + received,
                  completedBytes_ + qMax<qint64>(total, received),
                  files_.value(currentIndex_).relativePath);
}

void ModelDownloader::onFinished() {
    if (!reply_) return;
    const auto error = reply_->error();
    const QString errorString = reply_->errorString();
    onReadyRead();
    output_.flush();
    output_.close();
    reply_->deleteLater();
    reply_ = nullptr;

    if (cancelledByUser_ || error == QNetworkReply::OperationCanceledError) {
        QFile::remove(output_.fileName());
        emit cancelled();
        return;
    }
    if (error != QNetworkReply::NoError) {
        QFile::remove(output_.fileName());
        emit failed(QString("下载 %1 失败：%2")
                        .arg(files_.value(currentIndex_).relativePath, errorString));
        return;
    }

    const QString destination = QDir(resourcesRoot()).filePath(files_.at(currentIndex_).relativePath);
    QFile::remove(destination);
    if (!QFile::rename(output_.fileName(), destination)) {
        emit failed(QString("无法保存模型文件：%1").arg(destination));
        return;
    }
    completedBytes_ += QFileInfo(destination).size();
    ++currentIndex_;
    downloadNext();
}

} // namespace IntelNet
