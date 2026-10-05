// Rust 桥接层实现 - Qt 版本
#include "rust_bridge.h"
#include <iostream>
#include <thread>
#include <QJsonDocument>
#include <QJsonObject>
#include <chrono>

namespace IntelNet {

namespace {
template <typename Task>
void startAsync(const std::shared_ptr<RustBridge::AsyncState> &state, Task task) {
    if (state->stopping.load()) return;
    state->active.fetch_add(1);
    std::thread([state, task = std::move(task)]() mutable {
        task();
        state->active.fetch_sub(1);
    }).detach();
}
}

RustBridge::RustBridge()
    : initialized_(false), model_initialized_(false), asyncState_(std::make_shared<AsyncState>()) {
}

RustBridge::~RustBridge() {
    if (initialized_) {
        Shutdown();
    }
}

bool RustBridge::Initialize() {
    if (initialized_) {
        return true;
    }

    std::cout << "初始化 Rust 核心库..." << std::endl;
    int result = intelnet_init();

    if (result == 0) {
        initialized_ = true;
        std::cout << "✓ Rust 核心库初始化成功" << std::endl;
        return true;
    } else {
        std::cerr << "✗ Rust 核心库初始化失败: " << result << std::endl;
        return false;
    }
}

bool RustBridge::InitModel() {
    if (!initialized_) {
        std::cerr << "✗ 核心库未初始化" << std::endl;
        return false;
    }

    if (model_initialized_) {
        return true;
    }

    std::cout << "初始化 AI 模型..." << std::endl;
    int result = intelnet_init_model();

    if (result == 0) {
        model_initialized_ = true;
        std::cout << "✓ AI 模型初始化成功" << std::endl;
        return true;
    } else {
        std::cerr << "✗ AI 模型初始化失败: " << result << std::endl;
        return false;
    }
}

std::string RustBridge::AnalyzeImage(const std::string& image_data) {
    if (!initialized_) {
        return R"({"success": false, "error": "核心库未初始化"})";
    }

    std::cout << "开始分析图片..." << std::endl;
    char* result = intelnet_analyze_image(image_data.c_str());

    if (result == nullptr) {
        return R"({"success": false, "error": "分析失败"})";
    }

    std::string result_str(result);
    intelnet_free_string(result);

    std::cout << "图片分析完成" << std::endl;
    return result_str;
}

namespace {

// 简单判断 JSON 中的 "success" 是否为 true
bool jsonSuccess(const std::string& result) {
    return result.find("\"success\":true") != std::string::npos ||
           result.find("\"success\": true") != std::string::npos;
}

} // namespace

void RustBridge::AnalyzeImageAsync(
    const std::string& image_data,
    std::function<void(const std::string&, bool)> callback) {

    // 在新线程中执行，避免阻塞 UI
    const bool initialized = initialized_;
    startAsync(asyncState_, [image_data, callback, initialized]() {
        if (!initialized) {
            callback(R"({"success": false, "error": "核心库未初始化"})", false);
            return;
        }

        char* raw = intelnet_analyze_image_stream(image_data.c_str(), 1);
        std::string result = raw
            ? std::string(raw)
            : R"({"success": false, "error": "分析失败"})";
        if (raw) intelnet_free_string(raw);
        callback(result, jsonSuccess(result));
    });
}

void RustBridge::DescribeButtonAsync(
    const std::string& image_data,
    std::function<void(const std::string&, bool)> callback) {
    const bool initialized = initialized_;
    startAsync(asyncState_, [image_data, callback, initialized]() {
        if (!initialized) {
            callback(R"({"success": false, "error": "核心库未初始化"})", false);
            return;
        }
        char* raw = intelnet_describe_button_stream(image_data.c_str());
        std::string result = raw
            ? std::string(raw)
            : R"({"success": false, "error": "分析失败"})";
        if (raw) intelnet_free_string(raw);
        callback(result, jsonSuccess(result));
    });
}

void RustBridge::OcrCaptchaAsync(
    const std::string& image_data,
    std::function<void(const std::string&, bool)> callback) {
    const bool initialized = initialized_;
    startAsync(asyncState_, [image_data, callback, initialized]() {
        if (!initialized) {
            callback(R"({"success": false, "error": "核心库未初始化"})", false);
            return;
        }
        char* raw = intelnet_ocr_captcha(image_data.c_str(), 1);
        std::string result = raw
            ? std::string(raw)
            : R"({"success": false, "error": "OCR 分析失败"})";
        if (raw) intelnet_free_string(raw);
        callback(result, jsonSuccess(result));
    });
}

void RustBridge::ExplainFormAsync(
    const std::string& form_json,
    std::function<void(const std::string&, bool)> callback) {
    const bool initialized = initialized_;
    startAsync(asyncState_, [form_json, callback, initialized]() {
        if (!initialized) {
            callback(R"({"success": false, "error": "核心库未初始化"})", false);
            return;
        }
        char* raw = intelnet_explain_form(form_json.c_str(), 1);
        std::string result = raw
            ? std::string(raw)
            : R"({"success": false, "error": "表单解释失败"})";
        if (raw) intelnet_free_string(raw);
        callback(result, jsonSuccess(result));
    });
}

std::string RustBridge::SummarizeText(const std::string& text) {
    if (!initialized_) {
        return R"({"success": false, "error": "核心库未初始化"})";
    }

    std::cout << "开始总结文本..." << std::endl;
    char* result = intelnet_summarize_text(text.c_str());

    if (result == nullptr) {
        return R"({"success": false, "error": "总结失败"})";
    }

    std::string result_str(result);
    intelnet_free_string(result);

    std::cout << "文本总结完成" << std::endl;
    return result_str;
}

void RustBridge::SummarizeTextAsync(
    const std::string& text,
    std::function<void(const std::string&, bool)> callback) {

    // 在新线程中执行，避免阻塞 UI
    const bool initialized = initialized_;
    startAsync(asyncState_, [text, callback, initialized]() {
        std::string result = initialized
            ? ([&]() {
                char *raw = intelnet_summarize_text(text.c_str());
                std::string value = raw ? std::string(raw)
                    : R"({"success": false, "error": "总结失败"})";
                if (raw) intelnet_free_string(raw);
                return value;
            })()
            : R"({"success": false, "error": "核心库未初始化"})";
        callback(result, jsonSuccess(result));
    });
}

bool RustBridge::Speak(const std::string& text) {
    if (!initialized_) {
        std::cerr << "✗ 核心库未初始化" << std::endl;
        return false;
    }

    std::cout << "开始朗读: " << text.substr(0, 50) << "..." << std::endl;
    int result = intelnet_speak(text.c_str());
    return result == 0;
}

void RustBridge::StopSpeaking() {
    if (initialized_) {
        std::cout << "停止朗读" << std::endl;
        intelnet_stop_speaking();
    }
}

void RustBridge::Shutdown() {
    if (initialized_) {
        asyncState_->stopping.store(true);
        while (asyncState_->active.load() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::cout << "清理 Rust 核心库..." << std::endl;
        intelnet_shutdown();
        initialized_ = false;
        model_initialized_ = false;
        std::cout << "✓ Rust 核心库已清理" << std::endl;
    }
}

} // namespace IntelNet
