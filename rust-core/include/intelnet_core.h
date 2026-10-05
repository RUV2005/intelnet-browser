// IntelNet Core C API
// Auto-generated header for Rust FFI

#ifndef INTELNET_CORE_H
#define INTELNET_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

// 初始化 Rust 核心库
// 返回 0 表示成功，非 0 表示失败
int intelnet_init(void);

// 初始化 AI 模型
// 返回 0 表示成功，非 0 表示失败
int intelnet_init_model(void);

// 分析图片
// image_data: 图片数据（data URL、http URL 或本地路径）
// 返回 JSON 字符串，需要调用 intelnet_free_string 释放
char* intelnet_analyze_image(const char* image_data);
char* intelnet_analyze_image_stream(const char* image_data, int speak);
char* intelnet_describe_button_stream(const char* image_data);
char* intelnet_ocr_captcha(const char* image_data, int speak);
char* intelnet_explain_form(const char* form_json, int speak);

// 总结一段纯文本（网页内容）
// text: 网页正文
// 返回 JSON 字符串，需要调用 intelnet_free_string 释放
char* intelnet_summarize_text(const char* text);

// TTS 朗读文本
// text: 要朗读的文本
// 返回 0 表示成功，非 0 表示失败
int intelnet_speak(const char* text);

// 停止 TTS 朗读
void intelnet_stop_speaking(void);

// 释放 Rust 返回的字符串
void intelnet_free_string(char* ptr);

// 清理资源
void intelnet_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // INTELNET_CORE_H
