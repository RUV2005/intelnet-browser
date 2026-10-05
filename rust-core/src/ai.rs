use anyhow::{Context, Result};
use base64::Engine;
use image::imageops::FilterType;
use std::fs::File;
use std::io::{BufRead, BufReader, Cursor, Read, Write};
use std::net::TcpListener;
use std::path::{Path, PathBuf};
use std::process::{Child, Command, Stdio};
use std::sync::Mutex;
use std::time::{Duration, Instant};

/// 打印一行日志并立即 flush。
macro_rules! logln {
    ($($arg:tt)*) => {{
        println!($($arg)*);
        std::io::stdout().flush().ok();
    }};
}

/// 送进模型前，图片长边的上限（像素）。
/// 672 ≈ 360 个图像 token，画质接近原图；448 更快但会丢细节。
const MAX_EDGE: u32 = 448;
const OCR_MAX_EDGE: u32 = 336;
/// 单次回答最多生成的 token 数。
const MAX_TOKENS: u32 = 150;
/// 发给模型的提示词。
const PROMPT: &str = "请用中文详细描述这张图片。";
const BUTTON_PROMPT: &str =
    "这是一个网页按钮，只看图标样式，用中文一句话说出它的功能，只输出功能本身。";
const OCR_PROMPT: &str = "只输出图片中的文字，不要解释。";
/// 页面摘要提示词前缀。
const SUMMARY_PROMPT_PREFIX: &str =
    "以下是从网页中提取的结构化源码（包含 URL、标题、标题层级、正文、图片、链接等）。\
请用简洁的中文总结这个页面的主要内容，突出主题和关键信息，不要逐条罗列，控制在 200 字以内：\n\n";
const FORM_PROMPT_PREFIX: &str =
    "解释这个表单是干什么的，逐个说明字段含义，标出必填项。请用简洁中文回答：\n\n";
/// 页面摘要单次最多生成的 token 数。
const SUMMARY_MAX_TOKENS: u32 = 320;
/// 送入摘要模型的正文最大字符数（按上下文长度粗略裁剪）。
const MAX_SUMMARY_CHARS: usize = 3000;
/// 上下文长度。
const CTX_SIZE: u32 = 4096;

const SERVER_EXE: &str = "llama-server.exe";
const SERVER_START_TIMEOUT: Duration = Duration::from_secs(180);
const REQUEST_TIMEOUT: Duration = Duration::from_secs(300);
const HTTP_CONNECT_TIMEOUT: Duration = Duration::from_secs(10);

// ───────────────────────── 资源路径 ─────────────────────────

/// 找到包含指定标记文件的 resources 根目录。
/// 依次尝试：环境变量 INTELNET_RESOURCES → exe 旁边（打包后）→ 源码目录（开发时）。
pub fn resources_root_for(marker: &str) -> Result<PathBuf> {
    let mut candidates: Vec<PathBuf> = Vec::new();
    if let Some(p) = std::env::var_os("INTELNET_RESOURCES") {
        candidates.push(PathBuf::from(p));
    }
    if let Ok(exe) = std::env::current_exe() {
        if let Some(dir) = exe.parent() {
            candidates.push(dir.join("resources"));
            candidates.push(dir.to_path_buf());
        }
    }
    let manifest = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    candidates.push(manifest.join("resources"));
    // 开发时 rust-core 使用项目根目录共享 resources。
    if let Some(parent) = manifest.parent() {
        candidates.push(parent.join("resources"));
    }

    for c in &candidates {
        if c.join(marker).exists() {
            return Ok(c.clone());
        }
    }
    anyhow::bail!("找不到 {}，已尝试这些目录: {:?}", marker, candidates)
}

/// 找到包含 llama/ 和 models/ 的 resources 根目录。
fn resources_root() -> Result<PathBuf> {
    resources_root_for(&format!("llama/{}", SERVER_EXE))
}

// ───────────────────────── 图片处理 ─────────────────────────

fn http_client(timeout: Duration) -> Result<reqwest::blocking::Client> {
    reqwest::blocking::Client::builder()
        .connect_timeout(HTTP_CONNECT_TIMEOUT)
        .timeout(timeout)
        .build()
        .context("无法创建 HTTP 客户端")
}

/// 读取图片字节：支持 data URL、http(s) URL、本地路径。
fn load_image_bytes(image_data: &str) -> Result<Vec<u8>> {
    if image_data.starts_with("data:image") {
        let b64 = image_data.split(',').nth(1).context("无效的图片数据格式")?;
        base64::engine::general_purpose::STANDARD
            .decode(b64)
            .context("Base64 解码失败")
    } else if image_data.starts_with("http://") || image_data.starts_with("https://") {
        logln!("从网络下载图片: {}", image_data);
        let t = Instant::now();
        let bytes = http_client(Duration::from_secs(60))?
            .get(image_data)
            .send()
            .context("请求图片地址失败")?
            .error_for_status()
            .context("图片地址返回错误状态")?
            .bytes()
            .context("无法读取图片数据")?
            .to_vec();
        logln!(
            "图片下载耗时: {:?}，大小: {} 字节",
            t.elapsed(),
            bytes.len()
        );
        Ok(bytes)
    } else {
        std::fs::read(image_data).context("无法读取图片文件")
    }
}

/// 只缩小不放大，统一转成 RGB JPEG（去掉透明通道，也减小体积）。
fn shrink_for_model(bytes: &[u8]) -> Result<Vec<u8>> {
    shrink_for_model_with_edge(bytes, MAX_EDGE)
}

fn shrink_for_model_with_edge(bytes: &[u8], max_edge: u32) -> Result<Vec<u8>> {
    let img = image::load_from_memory(bytes).context("无法解析图片")?;
    let (w, h) = (img.width(), img.height());

    let img = if w.max(h) > max_edge {
        img.resize(max_edge, max_edge, FilterType::Lanczos3)
    } else {
        img
    };
    logln!("图片尺寸: {}x{} -> {}x{}", w, h, img.width(), img.height());

    let rgb = image::DynamicImage::ImageRgb8(img.to_rgb8());
    let mut out = Vec::new();
    rgb.write_to(&mut Cursor::new(&mut out), image::ImageFormat::Jpeg)
        .context("图片编码失败")?;
    Ok(out)
}

// ───────────────────────── llama-server 子进程 ─────────────────────────

struct LlamaServer {
    child: Child,
    base_url: String,
    client: reqwest::blocking::Client,
    log_path: PathBuf,
}

impl LlamaServer {
    fn start(
        model_rel: &str,
        mmproj_rel: &str,
        log_name: &str,
        model_root: Option<PathBuf>,
    ) -> Result<Self> {
        let root = resources_root()?;
        let llama_dir = root.join("llama");
        let server_exe = llama_dir.join(SERVER_EXE);
        let model_base = model_root.as_ref().unwrap_or(&root);
        let model = model_base.join(model_rel);
        let mmproj = model_base.join(mmproj_rel);

        for p in [&model, &mmproj] {
            if !p.exists() {
                anyhow::bail!("找不到模型文件: {:?}", p);
            }
        }

        // 让系统挑一个空闲端口。
        let port = TcpListener::bind("127.0.0.1:0")
            .context("无法分配本地端口")?
            .local_addr()?
            .port();

        let threads = std::thread::available_parallelism()
            .map(|n| n.get())
            .unwrap_or(4);

        let log_path = std::env::temp_dir().join(log_name);
        let log = File::create(&log_path).context("无法创建日志文件")?;
        let log_err = log.try_clone()?;

        logln!("启动 llama-server (端口 {}, 线程 {})...", port, threads);
        logln!("服务日志: {:?}", log_path);

        let mut cmd = Command::new(&server_exe);
        cmd.current_dir(&llama_dir)
            .arg("-m")
            .arg(&model)
            .arg("--mmproj")
            .arg(&mmproj)
            .arg("-t")
            .arg(threads.to_string())
            .arg("-c")
            .arg(CTX_SIZE.to_string())
            .arg("-np")
            .arg("1")
            .arg("--host")
            .arg("127.0.0.1")
            .arg("--port")
            .arg(port.to_string())
            .stdin(Stdio::null())
            .stdout(Stdio::from(log))
            .stderr(Stdio::from(log_err));

        // Windows 下不弹出黑色控制台窗口。
        #[cfg(windows)]
        {
            use std::os::windows::process::CommandExt;
            cmd.creation_flags(0x0800_0000); // CREATE_NO_WINDOW
        }

        let child = cmd.spawn().context("无法启动 llama-server")?;

        let mut server = Self {
            child,
            base_url: format!("http://127.0.0.1:{}", port),
            client: http_client(REQUEST_TIMEOUT)?,
            log_path,
        };
        server.wait_ready()?;
        Ok(server)
    }

    fn is_alive(&mut self) -> bool {
        matches!(self.child.try_wait(), Ok(None))
    }

    /// 轮询 /health，直到模型加载完成（200）。
    fn wait_ready(&mut self) -> Result<()> {
        let t = Instant::now();
        let probe = http_client(Duration::from_secs(2))?;
        let url = format!("{}/health", self.base_url);

        loop {
            if let Ok(Some(status)) = self.child.try_wait() {
                anyhow::bail!(
                    "llama-server 提前退出 ({:?})，日志末尾:\n{}",
                    status,
                    tail_of(&self.log_path, 20)
                );
            }
            if let Ok(resp) = probe.get(&url).send() {
                if resp.status().is_success() {
                    logln!("✓ llama-server 就绪，启动耗时 {:?}", t.elapsed());
                    return Ok(());
                }
            }
            if t.elapsed() > SERVER_START_TIMEOUT {
                anyhow::bail!(
                    "等待 llama-server 就绪超时 ({:?})，日志末尾:\n{}",
                    SERVER_START_TIMEOUT,
                    tail_of(&self.log_path, 20)
                );
            }
            std::thread::sleep(Duration::from_millis(300));
        }
    }

    /// 通用流式对话请求：content 直接作为 user 消息的 content 字段。
    /// 每收到一段文字就调用 on_delta，最后返回完整文本。
    fn stream_completion(
        &self,
        content: serde_json::Value,
        max_tokens: u32,
        temperature: f32,
        mut on_delta: impl FnMut(&str),
    ) -> Result<String> {
        let body = serde_json::json!({
            "messages": [{ "role": "user", "content": content }],
            "max_tokens": max_tokens,
            "temperature": temperature,
            "stream": true
        });

        let resp = self
            .client
            .post(format!("{}/v1/chat/completions", self.base_url))
            .json(&body)
            .send()
            .context("请求 llama-server 失败")?;

        let status = resp.status();
        if !status.is_success() {
            let text = resp.text().unwrap_or_default();
            anyhow::bail!("llama-server 返回 {}: {}", status, text);
        }

        let mut full = String::new();
        let mut first_token_logged = false;
        let t0 = Instant::now();

        for line in BufReader::new(resp).lines() {
            let line = line.context("读取流式响应失败")?;
            let Some(data) = line.strip_prefix("data: ") else {
                continue;
            };
            if data.trim() == "[DONE]" {
                break;
            }
            let v: serde_json::Value = match serde_json::from_str(data) {
                Ok(v) => v,
                Err(_) => continue,
            };
            if let Some(piece) = v["choices"][0]["delta"]["content"].as_str() {
                if !piece.is_empty() {
                    if !first_token_logged {
                        logln!("首个文字片段到达，距请求发出 {:?}", t0.elapsed());
                        first_token_logged = true;
                    }
                    full.push_str(piece);
                    on_delta(piece);
                }
            }
        }
        Ok(full.trim().to_string())
    }

    /// 流式请求：描述图片。
    fn describe_stream(
        &self,
        jpeg: &[u8],
        prompt: &str,
        max_tokens: u32,
        on_delta: impl FnMut(&str),
    ) -> Result<String> {
        let b64 = base64::engine::general_purpose::STANDARD.encode(jpeg);
        let content = serde_json::json!([
            { "type": "text", "text": prompt },
            { "type": "image_url",
              "image_url": { "url": format!("data:image/jpeg;base64,{}", b64) } }
        ]);
        self.stream_completion(content, max_tokens, 0.2, on_delta)
    }

    /// 流式请求：总结纯文本（网页内容）。
    fn summarize_stream(&self, text: &str, on_delta: impl FnMut(&str)) -> Result<String> {
        let trimmed: String = text.chars().take(MAX_SUMMARY_CHARS).collect();
        let content = serde_json::Value::String(format!("{}{}", SUMMARY_PROMPT_PREFIX, trimmed));
        self.stream_completion(content, SUMMARY_MAX_TOKENS, 0.3, on_delta)
    }

    #[allow(dead_code)]
    fn describe(&self, jpeg: &[u8]) -> Result<String> {
        self.describe_stream(jpeg, PROMPT, MAX_TOKENS, |_| {})
    }
}

impl Drop for LlamaServer {
    fn drop(&mut self) {
        let _ = self.child.kill();
        let _ = self.child.wait();
        logln!("llama-server 已停止");
    }
}

/// 读取文件最后 n 行，用于报错时展示。
fn tail_of(path: &Path, n: usize) -> String {
    let mut s = String::new();
    if let Ok(mut f) = File::open(path) {
        let _ = f.read_to_string(&mut s);
    }
    let lines: Vec<&str> = s.lines().collect();
    lines[lines.len().saturating_sub(n)..].join("\n")
}

// ───────────────────────── 对外接口（与旧版保持一致） ─────────────────────────

pub struct ModelManager {
    server: Mutex<Option<LlamaServer>>,
    ocr_server: Mutex<Option<LlamaServer>>,
}

impl ModelManager {
    pub fn new() -> Self {
        Self {
            server: Mutex::new(None),
            ocr_server: Mutex::new(None),
        }
    }

    fn lock(&self) -> std::sync::MutexGuard<'_, Option<LlamaServer>> {
        self.server.lock().unwrap_or_else(|e| e.into_inner())
    }

    /// 确保服务在运行；已死则重启。
    fn ensure_running(
        slot: &mut Option<LlamaServer>,
        model_rel: &str,
        mmproj_rel: &str,
        log_name: &str,
        model_root: Option<PathBuf>,
    ) -> Result<()> {
        let alive = slot.as_mut().map(|s| s.is_alive()).unwrap_or(false);
        if !alive {
            if slot.is_some() {
                logln!("llama-server 已退出，正在重启...");
            }
            *slot = None; // 先释放旧的
            *slot = Some(LlamaServer::start(
                model_rel, mmproj_rel, log_name, model_root,
            )?);
        }
        Ok(())
    }

    pub fn init_model(&self) -> Result<()> {
        let mut guard = self.lock();
        Self::ensure_running(
            &mut guard,
            "models/qwen2vl/model.gguf",
            "models/qwen2vl/mmproj.gguf",
            "intelnet-llama-server.log",
            None,
        )
    }

    pub fn analyze_image(&self, image_data: &str) -> Result<String> {
        self.analyze_image_stream(image_data, |_| {})
    }

    /// 流式分析：每生成一段文字就调用 on_delta
    pub fn analyze_image_stream(
        &self,
        image_data: &str,
        on_delta: impl FnMut(&str),
    ) -> Result<String> {
        let t_total = Instant::now();

        // 先做图片处理，失败就不必启动服务。
        let raw = load_image_bytes(image_data)?;
        let t = Instant::now();
        let jpeg = shrink_for_model(&raw)?;
        logln!(
            "图片预处理耗时: {:?}，发送 {} 字节",
            t.elapsed(),
            jpeg.len()
        );

        let mut guard = self.lock();
        Self::ensure_running(
            &mut guard,
            "models/qwen2vl/model.gguf",
            "models/qwen2vl/mmproj.gguf",
            "intelnet-llama-server.log",
            None,
        )?;

        let t = Instant::now();
        let text = guard
            .as_ref()
            .context("服务未运行")?
            .describe_stream(&jpeg, PROMPT, MAX_TOKENS, on_delta)?;
        logln!(
            "推理耗时: {:?}，总耗时: {:?}",
            t.elapsed(),
            t_total.elapsed()
        );
        Ok(text)
    }

    pub fn analyze_button_stream(
        &self,
        image_data: &str,
        on_delta: impl FnMut(&str),
    ) -> Result<String> {
        let raw = load_image_bytes(image_data)?;
        let jpeg = shrink_for_model(&raw)?;
        let mut guard = self.lock();
        Self::ensure_running(
            &mut guard,
            "models/qwen2vl/model.gguf",
            "models/qwen2vl/mmproj.gguf",
            "intelnet-llama-server.log",
            None,
        )?;
        guard
            .as_ref()
            .context("服务未运行")?
            .describe_stream(&jpeg, BUTTON_PROMPT, 48, on_delta)
    }

    pub fn ocr_captcha(&self, image_data: &str) -> Result<String> {
        let raw = load_image_bytes(image_data)?;
        let jpeg = shrink_for_model_with_edge(&raw, OCR_MAX_EDGE)?;
        let mut guard = self.ocr_server.lock().unwrap_or_else(|e| e.into_inner());
        Self::ensure_running(
            &mut guard,
            "models/paddleocr-vl/PaddleOCR-VL-1.6-GGUF.gguf",
            "models/paddleocr-vl/PaddleOCR-VL-1.6-GGUF-mmproj.gguf",
            "intelnet-ocr-server.log",
            None,
        )?;
        guard
            .as_ref()
            .context("OCR 服务未运行")?
            .describe_stream(&jpeg, OCR_PROMPT, 32, |_| {})
    }

    /// 总结一段纯文本（网页内容）。
    pub fn summarize_text(&self, text: &str) -> Result<String> {
        self.summarize_text_stream(text, |_| {})
    }

    /// 流式总结：每生成一段文字就调用 on_delta。
    pub fn summarize_text_stream(&self, text: &str, on_delta: impl FnMut(&str)) -> Result<String> {
        let t_total = Instant::now();
        let mut guard = self.lock();
        Self::ensure_running(
            &mut guard,
            "models/qwen2vl/model.gguf",
            "models/qwen2vl/mmproj.gguf",
            "intelnet-llama-server.log",
            None,
        )?;

        let t = Instant::now();
        let result = guard
            .as_ref()
            .context("服务未运行")?
            .summarize_stream(text, on_delta)?;
        logln!(
            "摘要推理耗时: {:?}，总耗时: {:?}",
            t.elapsed(),
            t_total.elapsed()
        );
        Ok(result)
    }

    pub fn explain_form(&self, form_json: &str) -> Result<String> {
        let mut guard = self.lock();
        Self::ensure_running(
            &mut guard,
            "models/qwen2vl/model.gguf",
            "models/qwen2vl/mmproj.gguf",
            "intelnet-llama-server.log",
            None,
        )?;
        let content = serde_json::Value::String(format!("{}{}", FORM_PROMPT_PREFIX, form_json));
        guard
            .as_ref()
            .context("服务未运行")?
            .stream_completion(content, 256, 0.2, |_| {})
    }

    /// 应用退出时调用，结束 llama-server 子进程。
    pub fn shutdown(&self) {
        let mut guard = self.lock();
        *guard = None;
        let mut ocr_guard = self.ocr_server.lock().unwrap_or_else(|e| e.into_inner());
        *ocr_guard = None;
    }
}
