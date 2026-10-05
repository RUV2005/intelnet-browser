// Rust 桥接层 - Qt 版本
#ifndef RUST_BRIDGE_H
#define RUST_BRIDGE_H

#include <string>
#include <functional>

// Rust C FFI 函数声明
extern "C" {
    int intelnet_init();
    int intelnet_init_model();
    char* intelnet_analyze_image(const char* image_data);
    char* intelnet_analyze_image_stream(const char* image_data);
    char* intelnet_summarize_text(const char* text);
    int intelnet_speak(const char* text);
    void intelnet_stop_speaking();
    void intelnet_free_string(char* ptr);
    void intelnet_shutdown();
}

namespace IntelNet {

class RustBridge {
public:
    RustBridge();
    ~RustBridge();

    // 初始化
    bool Initialize();

    // 初始化 AI 模型
    bool InitModel();

    // 分析图片（同步）
    std::string AnalyzeImage(const std::string& image_data);

    // 分析图片（异步，带回调）
    void AnalyzeImageAsync(const std::string& image_data,
                          std::function<void(const std::string&, bool)> callback);

    // 总结网页文本（同步）
    std::string SummarizeText(const std::string& text);

    // 总结网页文本（异步，带回调）
    void SummarizeTextAsync(const std::string& text,
                           std::function<void(const std::string&, bool)> callback);

    // TTS 朗读
    bool Speak(const std::string& text);

    // 停止朗读
    void StopSpeaking();

    // 清理资源
    void Shutdown();

private:
    bool initialized_;
    bool model_initialized_;
};

} // namespace IntelNet

#endif // RUST_BRIDGE_H
