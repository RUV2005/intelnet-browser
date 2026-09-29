// Rust 核心库 - C FFI 接口
use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::sync::Mutex;

mod ai;
mod tts;

// 全局状态管理
static MODEL_MANAGER: Mutex<Option<ai::ModelManager>> = Mutex::new(None);
static TTS_PLAYER: Mutex<Option<tts::TtsPlayer>> = Mutex::new(None);

/// 初始化 Rust 核心库
/// 返回 0 表示成功，非 0 表示失败
#[no_mangle]
pub extern "C" fn intelnet_init() -> i32 {
    println!("IntelNet Core 初始化中...");

    let mut model = MODEL_MANAGER.lock().unwrap();
    *model = Some(ai::ModelManager::new());

    let mut tts = TTS_PLAYER.lock().unwrap();
    *tts = Some(tts::TtsPlayer::spawn());

    println!("✓ IntelNet Core 初始化成功");
    0
}

/// 初始化 AI 模型
/// 返回 0 表示成功，非 0 表示失败
#[no_mangle]
pub extern "C" fn intelnet_init_model() -> i32 {
    println!("初始化 AI 模型...");

    let model = MODEL_MANAGER.lock().unwrap();
    if let Some(manager) = model.as_ref() {
        match manager.init_model() {
            Ok(_) => {
                println!("✓ AI 模型初始化成功");
                0
            }
            Err(e) => {
                eprintln!("✗ AI 模型初始化失败: {}", e);
                -1
            }
        }
    } else {
        eprintln!("✗ ModelManager 未初始化");
        -2
    }
}

/// 分析图片
/// image_data: 图片数据（data URL、http URL 或本地路径）
/// 返回 JSON 字符串，需要调用 intelnet_free_string 释放
#[no_mangle]
pub extern "C" fn intelnet_analyze_image(image_data: *const c_char) -> *mut c_char {
    if image_data.is_null() {
        return std::ptr::null_mut();
    }

    let c_str = unsafe { CStr::from_ptr(image_data) };
    let image_str = match c_str.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };

    println!("分析图片: {}", &image_str[..image_str.len().min(100)]);

    let model = MODEL_MANAGER.lock().unwrap();
    if let Some(manager) = model.as_ref() {
        match manager.analyze_image(image_str) {
            Ok(result) => {
                // 返回 JSON 格式
                let json = serde_json::json!({
                    "success": true,
                    "result": result
                });

                match CString::new(json.to_string()) {
                    Ok(c_string) => c_string.into_raw(),
                    Err(_) => std::ptr::null_mut(),
                }
            }
            Err(e) => {
                let json = serde_json::json!({
                    "success": false,
                    "error": format!("{}", e)
                });

                match CString::new(json.to_string()) {
                    Ok(c_string) => c_string.into_raw(),
                    Err(_) => std::ptr::null_mut(),
                }
            }
        }
    } else {
        std::ptr::null_mut()
    }
}

/// 总结一段纯文本（网页内容）
/// text: 网页正文
/// 返回 JSON 字符串，需要调用 intelnet_free_string 释放
#[no_mangle]
pub extern "C" fn intelnet_summarize_text(text: *const c_char) -> *mut c_char {
    if text.is_null() {
        return std::ptr::null_mut();
    }

    let c_str = unsafe { CStr::from_ptr(text) };
    let text_str = match c_str.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };

    println!("总结文本，长度: {} 字符", text_str.chars().count());

    let model = MODEL_MANAGER.lock().unwrap();
    if let Some(manager) = model.as_ref() {
        match manager.summarize_text(text_str) {
            Ok(result) => {
                let json = serde_json::json!({
                    "success": true,
                    "result": result
                });

                match CString::new(json.to_string()) {
                    Ok(c_string) => c_string.into_raw(),
                    Err(_) => std::ptr::null_mut(),
                }
            }
            Err(e) => {
                let json = serde_json::json!({
                    "success": false,
                    "error": format!("{}", e)
                });

                match CString::new(json.to_string()) {
                    Ok(c_string) => c_string.into_raw(),
                    Err(_) => std::ptr::null_mut(),
                }
            }
        }
    } else {
        std::ptr::null_mut()
    }
}

/// TTS 朗读文本
/// text: 要朗读的文本
/// 返回 0 表示成功，非 0 表示失败
#[no_mangle]
pub extern "C" fn intelnet_speak(text: *const c_char) -> i32 {
    if text.is_null() {
        return -1;
    }

    let c_str = unsafe { CStr::from_ptr(text) };
    let text_str = match c_str.to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    let tts = TTS_PLAYER.lock().unwrap();
    if let Some(player) = tts.as_ref() {
        player.speak(text_str.to_string());
        0
    } else {
        -2
    }
}

/// 停止 TTS 朗读
#[no_mangle]
pub extern "C" fn intelnet_stop_speaking() {
    let tts = TTS_PLAYER.lock().unwrap();
    if let Some(player) = tts.as_ref() {
        player.stop();
    }
}

/// 释放 Rust 返回的字符串
#[no_mangle]
pub extern "C" fn intelnet_free_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe {
            let _ = CString::from_raw(ptr);
        }
    }
}

/// 清理资源
#[no_mangle]
pub extern "C" fn intelnet_shutdown() {
    println!("IntelNet Core 关闭中...");

    let mut model = MODEL_MANAGER.lock().unwrap();
    if let Some(manager) = model.take() {
        manager.shutdown();
    }

    let mut tts = TTS_PLAYER.lock().unwrap();
    *tts = None;

    println!("✓ IntelNet Core 已关闭");
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    #[test]
    fn test_init_and_shutdown() {
        assert_eq!(intelnet_init(), 0);
        intelnet_shutdown();
    }

    #[test]
    fn test_free_string() {
        let test_str = CString::new("test string").unwrap();
        let raw_ptr = test_str.into_raw();

        // Should not crash
        intelnet_free_string(raw_ptr);
    }

    #[test]
    fn test_free_null_string() {
        // Should handle null gracefully
        intelnet_free_string(std::ptr::null_mut());
    }

    #[test]
    fn test_speak_null_text() {
        assert_eq!(intelnet_init(), 0);
        assert_eq!(intelnet_speak(std::ptr::null()), -1);
        intelnet_shutdown();
    }

    #[test]
    fn test_analyze_null_image() {
        assert_eq!(intelnet_init(), 0);
        let result = intelnet_analyze_image(std::ptr::null());
        assert!(result.is_null());
        intelnet_shutdown();
    }

    #[test]
    fn test_summarize_null_text() {
        assert_eq!(intelnet_init(), 0);
        let result = intelnet_summarize_text(std::ptr::null());
        assert!(result.is_null());
        intelnet_shutdown();
    }

    #[test]
    fn test_multiple_init() {
        // Multiple inits should not crash
        assert_eq!(intelnet_init(), 0);
        assert_eq!(intelnet_init(), 0);
        intelnet_shutdown();
    }

    #[test]
    fn test_stop_speaking() {
        assert_eq!(intelnet_init(), 0);
        // Should not crash even if nothing is playing
        intelnet_stop_speaking();
        intelnet_shutdown();
    }
}
