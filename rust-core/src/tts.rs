use anyhow::{Context, Result};
use std::collections::HashMap;
use std::io::Write;
use std::path::PathBuf;
use std::process::{Child, Command, Stdio};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::mpsc::{channel, Sender};
use std::sync::{Arc, Mutex};

// ───────────────────────── 错误日志 ─────────────────────────
// release 版没有控制台，eprintln 会丢进黑洞；关键失败同时写文件方便排错
fn tts_log(msg: &str) {
    let ts = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0);
    let path = std::env::temp_dir().join("intelnet_tts.log");
    if let Ok(mut f) = std::fs::OpenOptions::new()
        .create(true)
        .append(true)
        .open(path)
    {
        let _ = writeln!(f, "[{ts}] {msg}");
    }
}

// ───────────────────────── 切句器 ─────────────────────────

pub struct SentenceSplitter {
    buf: String,
}

impl SentenceSplitter {
    const MIN_CHARS: usize = 8; // 太短的片段并入下一句
    const MAX_CHARS: usize = 60; // 超长时在逗号处强行切开

    pub fn new() -> Self {
        Self { buf: String::new() }
    }

    /// 喂入新文字，返回已经凑齐的完整句子（可能 0 个或多个）
    pub fn push(&mut self, piece: &str) -> Vec<String> {
        self.buf.push_str(piece);
        let mut out = Vec::new();

        loop {
            let chars: Vec<char> = self.buf.chars().collect();
            let mut cut: Option<usize> = None;

            for i in 0..chars.len() {
                let n = i + 1;
                if n < Self::MIN_CHARS {
                    continue;
                }
                let c = chars[i];
                let next_ws = chars.get(i + 1).is_some_and(|x| x.is_whitespace());
                let prev_digit = i > 0 && chars[i - 1].is_ascii_digit();

                let hard = matches!(c, '。' | '！' | '？' | '；' | '\n')
                    || (matches!(c, '.' | '!' | '?') && next_ws && !prev_digit);
                let soft = n >= Self::MAX_CHARS && matches!(c, '，' | ',' | '、');
                let forced = n >= Self::MAX_CHARS * 2;

                if hard || soft || forced {
                    cut = Some(n);
                    break;
                }
            }

            match cut {
                Some(n) => {
                    let sentence: String = chars[..n].iter().collect();
                    self.buf = chars[n..].iter().collect();
                    let s = clean_for_speech(&sentence);
                    if !s.is_empty() {
                        out.push(s);
                    }
                }
                None => break,
            }
        }
        out
    }

    /// 流结束时取出剩余内容
    pub fn finish(&mut self) -> Option<String> {
        let s = clean_for_speech(&self.buf);
        self.buf.clear();
        if s.is_empty() {
            None
        } else {
            Some(s)
        }
    }
}

/// 去掉 Markdown 符号等不该念出来的字符
fn clean_for_speech(text: &str) -> String {
    text.chars()
        .map(|c| match c {
            '*' | '#' | '`' | '_' | '~' | '>' | '|' | '\n' | '\r' => ' ',
            c => c,
        })
        .collect::<String>()
        .trim()
        .to_string()
}

// ───────────────────────── Piper 工作池（并行合成） ─────────────────────────

const WORKER_COUNT: usize = 3; // 并行工作进程数

struct PiperWorker {
    child: Child,
}

impl PiperWorker {
    fn spawn(model_path: &PathBuf) -> Result<Self> {
        let root = crate::ai::resources_root_for("piper/zh_CN-chaowei-medium.onnx")?;

        // 优先用安装包自带的 portable python，找不到再回退系统 python
        let python_exe = {
            let bundled = root.join("python").join("python.exe");
            if bundled.is_file() {
                bundled
            } else {
                which::which("python")
                    .or_else(|_| which::which("python3"))
                    .context("找不到 python 或 python3")?
            }
        };

        let script_path = root.join("tts_synthesize.py");

        let mut cmd = Command::new(&python_exe);
        cmd.arg(&script_path)
            .arg(model_path)
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .stderr(Stdio::inherit());

        #[cfg(windows)]
        {
            use std::os::windows::process::CommandExt;
            cmd.creation_flags(0x0800_0000); // CREATE_NO_WINDOW
        }

        let mut child = cmd.spawn().context("无法启动 Python TTS 工作进程")?;

        // 预热：发送一个简短的测试文本，强制加载所有模型
        // 注意：预热失败直接 bail，不把死 worker 放进池子，
        // 否则之后每次合成都静默失败（UI 显示在读但没声）
        let warmup_text = "预热";
        let temp_file = std::env::temp_dir().join(format!(
            "piper_warmup_{:?}.txt",
            std::thread::current().id()
        ));
        std::fs::write(&temp_file, warmup_text.as_bytes()).ok();

        let warmup_result: Result<()> = (|| {
            let stdin = child.stdin.as_mut().context("无法获取 stdin")?;
            writeln!(stdin, "{}", temp_file.display()).context("预热写入失败")?;
            stdin.flush().context("预热 flush 失败")?;

            let stdout = child.stdout.as_mut().context("无法获取 stdout")?;
            let mut len_buf = [0u8; 4];
            std::io::Read::read_exact(stdout, &mut len_buf).context("预热读取长度失败")?;
            let len = u32::from_le_bytes(len_buf) as usize;
            if len == 0 {
                anyhow::bail!("预热返回空结果（Python 脚本可能崩了）");
            }
            let mut buf = vec![0u8; len];
            std::io::Read::read_exact(stdout, &mut buf).context("预热读取 PCM 失败")?;
            Ok(())
        })();

        let _ = std::fs::remove_file(&temp_file);

        if let Err(e) = warmup_result {
            let msg = format!("TTS 工作进程预热失败: {e:#}");
            eprintln!("{msg}");
            tts_log(&msg);
            return Err(e);
        }

        Ok(Self { child })
    }

    fn synthesize(&mut self, text: &str) -> Result<Vec<i16>> {
        // 写文本到临时文件
        let temp_file = std::env::temp_dir().join(format!(
            "piper_input_{}_{:?}.txt",
            std::process::id(),
            std::thread::current().id()
        ));
        std::fs::write(&temp_file, text.as_bytes()).context("无法写入临时文本文件")?;

        // 发送文件路径到 stdin
        {
            let stdin = self.child.stdin.as_mut().context("无法获取 stdin")?;
            writeln!(stdin, "{}", temp_file.display()).context("写入请求失败")?;
            stdin.flush().context("flush 失败")?;
        }

        // 读取响应：先读 4 字节长度，再读 PCM 数据
        let stdout = self.child.stdout.as_mut().context("无法获取 stdout")?;
        let mut len_buf = [0u8; 4];
        std::io::Read::read_exact(stdout, &mut len_buf).context("读取响应长度失败")?;
        let len = u32::from_le_bytes(len_buf) as usize;

        // 清理临时文件
        let _ = std::fs::remove_file(&temp_file);

        if len == 0 {
            anyhow::bail!("TTS 合成返回空结果");
        }

        let mut pcm_buf = vec![0u8; len];
        std::io::Read::read_exact(stdout, &mut pcm_buf).context("读取 PCM 数据失败")?;

        // 转换为 i16 采样
        let samples: Vec<i16> = pcm_buf
            .as_chunks::<2>()
            .0
            .iter()
            .map(|b| i16::from_le_bytes(*b))
            .collect();

        Ok(samples)
    }
}

impl Drop for PiperWorker {
    fn drop(&mut self) {
        let _ = self.child.kill();
        let _ = self.child.wait();
    }
}

pub struct PiperEngine {
    workers: Arc<Mutex<Vec<PiperWorker>>>,
    sample_rate: u32,
}

impl PiperEngine {
    pub fn new() -> Result<Self> {
        let root = crate::ai::resources_root_for("piper/zh_CN-chaowei-medium.onnx")?;
        let model_path = root.join("piper").join("zh_CN-chaowei-medium.onnx");

        if !model_path.exists() {
            anyhow::bail!("找不到 Piper 模型: {:?}", model_path);
        }

        // 读取采样率
        let config_path = model_path.with_extension("onnx.json");
        let cfg: serde_json::Value = serde_json::from_str(
            &std::fs::read_to_string(&config_path).context("读取模型配置失败")?,
        )
        .context("模型配置不是有效 JSON")?;
        let sample_rate = cfg["audio"]["sample_rate"].as_u64().unwrap_or(22050) as u32;

        // 启动多个工作进程（并行启动和预热）
        println!("启动 {} 个 Piper 工作进程...", WORKER_COUNT);

        use std::sync::mpsc::channel;
        let (tx, rx) = channel();

        // 并行启动所有 worker
        for i in 0..WORKER_COUNT {
            let model_path_clone = model_path.clone();
            let tx_clone = tx.clone();

            std::thread::spawn(move || {
                let t = std::time::Instant::now();
                match PiperWorker::spawn(&model_path_clone) {
                    Ok(w) => {
                        println!(
                            "  工作进程 {} 已启动并预热完成，耗时 {:?}",
                            i + 1,
                            t.elapsed()
                        );
                        tx_clone.send(Ok(w)).ok();
                    }
                    Err(e) => {
                        let msg = format!("工作进程 {} 启动失败: {:#}", i + 1, e);
                        eprintln!("  {msg}");
                        tts_log(&format!("TTS {msg}"));
                        tx_clone.send(Err(e)).ok();
                    }
                }
            });
        }

        drop(tx); // 关闭发送端

        // 收集所有启动好的 worker
        let mut workers = Vec::new();
        for w in rx.into_iter().flatten() {
            workers.push(w);
        }

        if workers.is_empty() {
            let msg = "无法启动任何 TTS 工作进程".to_string();
            tts_log(&format!("TTS {msg}"));
            anyhow::bail!(msg);
        }

        println!(
            "✓ Piper 引擎就绪，{} 个工作进程，采样率 {} Hz",
            workers.len(),
            sample_rate
        );
        Ok(Self {
            workers: Arc::new(Mutex::new(workers)),
            sample_rate,
        })
    }

    /// 合成一句话，返回 (采样数据, 采样率)
    pub fn synthesize(&self, text: &str) -> Result<(Vec<i16>, u32)> {
        let text = clean_for_speech(text);
        if text.is_empty() {
            return Ok((Vec::new(), self.sample_rate));
        }

        // 等待并取一个空闲的 worker（阻塞直到有可用的）
        let mut worker = loop {
            let mut workers = self.workers.lock().unwrap();
            if let Some(w) = workers.pop() {
                break w;
            }
            drop(workers); // 释放锁
            std::thread::sleep(std::time::Duration::from_millis(10)); // 等待一小会
        };

        // 合成
        let result = worker.synthesize(&text);

        // 立即归还 worker
        self.workers.lock().unwrap().push(worker);

        let samples = result?;
        Ok((samples, self.sample_rate))
    }
}

// ───────────────────────── 播放队列（保证顺序播放） ─────────────────────────

enum Cmd {
    Speak(u64, u64, String),             // (代次, 句子编号, 文本)
    SpeakReady(u64, u64, Vec<i16>, u32), // (代次, 句子编号, 采样数据, 采样率)
    Clear,
}

pub struct TtsPlayer {
    tx: Sender<Cmd>,
    epoch: Arc<AtomicU64>,
    next_seq: Arc<AtomicU64>, // 下一个句子的编号
    ready: Arc<std::sync::atomic::AtomicBool>,
}

impl TtsPlayer {
    /// 在专用线程里创建引擎和音频设备
    pub fn spawn() -> Self {
        let (tx, rx) = channel::<Cmd>();
        let epoch = Arc::new(AtomicU64::new(0));
        let next_seq = Arc::new(AtomicU64::new(0));
        let ready = Arc::new(std::sync::atomic::AtomicBool::new(false));
        let epoch_t = epoch.clone();
        let tx_clone = tx.clone(); // 克隆一份给子线程内部使用
        let ready_t = ready.clone();

        std::thread::spawn(move || {
            let engine = match PiperEngine::new() {
                Ok(e) => e,
                Err(e) => {
                    let msg = format!("TTS 引擎初始化失败: {e:#}");
                    eprintln!("{msg}");
                    tts_log(&msg);
                    return;
                }
            };

            let (_stream, handle) = match rodio::OutputStream::try_default() {
                Ok(v) => v,
                Err(e) => {
                    let msg = format!("无法打开音频设备: {e}");
                    eprintln!("{msg}");
                    tts_log(&msg);
                    return;
                }
            };

            let sink = match rodio::Sink::try_new(&handle) {
                Ok(s) => s,
                Err(e) => {
                    let msg = format!("无法创建播放队列: {e}");
                    eprintln!("{msg}");
                    tts_log(&msg);
                    return;
                }
            };
            ready_t.store(true, Ordering::SeqCst);

            // 顺序播放缓冲区：存储已合成但还没到播放顺序的句子
            let mut pending: HashMap<u64, (Vec<i16>, u32)> = HashMap::new();
            let mut next_play_seq = 0u64; // 下一个应该播放的句子编号

            for cmd in rx {
                match cmd {
                    Cmd::Clear => {
                        sink.clear();
                        sink.play();
                        pending.clear();
                        next_play_seq = 0;
                    }
                    Cmd::Speak(ep, seq, text) => {
                        // 已被打断，丢弃过期的句子
                        if ep != epoch_t.load(Ordering::SeqCst) {
                            continue;
                        }

                        // 并行合成（在新线程里，不阻塞主播放循环）
                        let engine = engine.clone();
                        let epoch_check = epoch_t.clone();
                        let tx_result = tx_clone.clone();

                        std::thread::spawn(move || {
                            let t = std::time::Instant::now();
                            match engine.synthesize(&text) {
                                Ok((samples, rate)) => {
                                    println!(
                                        "[tts] 句子 {} 合成 {} 字耗时 {:?}",
                                        seq,
                                        text.chars().count(),
                                        t.elapsed()
                                    );
                                    // 合成期间可能又被打断，再检查一次
                                    if !samples.is_empty()
                                        && ep == epoch_check.load(Ordering::SeqCst)
                                    {
                                        // 发送结果回主线程
                                        let _ =
                                            tx_result.send(Cmd::SpeakReady(ep, seq, samples, rate));
                                    }
                                }
                                Err(e) => {
                                    let msg = format!("[tts] 句子 {seq} 合成失败: {e:#}");
                                    eprintln!("{msg}");
                                    tts_log(&msg);
                                }
                            }
                        });
                    }
                    Cmd::SpeakReady(ep, seq, samples, rate) => {
                        // 已被打断
                        if ep != epoch_t.load(Ordering::SeqCst) {
                            continue;
                        }

                        // 如果是下一个应该播放的句子，立即播放
                        if seq == next_play_seq {
                            sink.append(rodio::buffer::SamplesBuffer::new(1, rate, samples));
                            next_play_seq += 1;

                            // 检查缓冲区里有没有后续的句子可以连续播放
                            while let Some((s, r)) = pending.remove(&next_play_seq) {
                                sink.append(rodio::buffer::SamplesBuffer::new(1, r, s));
                                next_play_seq += 1;
                            }
                        } else {
                            // 还没轮到，先缓存起来
                            pending.insert(seq, (samples, rate));
                        }
                    }
                }
            }
        });

        Self {
            tx,
            epoch,
            next_seq,
            ready,
        }
    }

    /// 排队朗读一句；播放队列会按顺序无缝播放
    pub fn speak(&self, sentence: String) -> bool {
        if !self.ready.load(Ordering::SeqCst) {
            return false;
        }
        let ep = self.epoch.load(Ordering::SeqCst);
        let seq = self.next_seq.fetch_add(1, Ordering::SeqCst);
        self.tx.send(Cmd::Speak(ep, seq, sentence)).is_ok()
    }

    /// 打断：使排队中的句子作废，并清空已在播放队列里的音频
    pub fn stop(&self) {
        self.epoch.fetch_add(1, Ordering::SeqCst);
        self.next_seq.store(0, Ordering::SeqCst);
        let _ = self.tx.send(Cmd::Clear);
    }
}

// 为了能在 std::thread::spawn 里使用 PiperEngine，需要实现 Clone
impl Clone for PiperEngine {
    fn clone(&self) -> Self {
        Self {
            workers: self.workers.clone(),
            sample_rate: self.sample_rate,
        }
    }
}
