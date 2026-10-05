// Rust 桥接层实现 - Qt 版本
#include "rust_bridge.h"
#include <iostream>
#include <thread>
#include <QJsonDocument>
#include <QJsonObject>

namespace IntelNet {

RustBridge::RustBridge()
    : initialized_(false), model_initialized_(false) {
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
    std::thread([this, image_data, callback]() {
        if (!initialized_) {
            callback(R"({"success": false, "error": "核心库未初始化"})", false);
            return;
        }

        char* raw = intelnet_analyze_image_stream(image_data.c_str(), 1);
        std::string result = raw
            ? std::string(raw)
            : R"({"success": false, "error": "分析失败"})";
        if (raw) intelnet_free_string(raw);
        callback(result, jsonSuccess(result));
    }).detach();
}

void RustBridge::DescribeButtonAsync(
    const std::string& image_data,
    std::function<void(const std::string&, bool)> callback) {
    std::thread([this, image_data, callback]() {
        if (!initialized_) {
            callback(R"({"success": false, "error": "核心库未初始化"})", false);
            return;
        }
        char* raw = intelnet_describe_button_stream(image_data.c_str());
        std::string result = raw
            ? std::string(raw)
            : R"({"success": false, "error": "分析失败"})";
        if (raw) intelnet_free_string(raw);
        callback(result, jsonSuccess(result));
    }).detach();
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
    std::thread([this, text, callback]() {
        std::string result = SummarizeText(text);
        callback(result, jsonSuccess(result));
    }).detach();
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
        std::cout << "清理 Rust 核心库..." << std::endl;
        intelnet_shutdown();
        initialized_ = false;
        model_initialized_ = false;
        std::cout << "✓ Rust 核心库已清理" << std::endl;
    }
}

} // namespace IntelNet
