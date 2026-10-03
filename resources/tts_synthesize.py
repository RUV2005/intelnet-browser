#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Piper TTS 常驻合成服务
接受来自 stdin 的合成请求（每行一个文件路径），输出 PCM 音频到 stdout
协议：
  输入: <text_file_path>\n
  输出: <length:u32 little-endian><pcm_bytes>
"""
import sys
import struct
from pathlib import Path
from piper.voice import PiperVoice

def main():
    if len(sys.argv) < 2:
        print("用法: tts_synthesize.py <model_path>", file=sys.stderr)
        sys.exit(1)

    model_path = Path(sys.argv[1])
    if not model_path.exists():
        print(f"错误: 模型文件不存在: {model_path}", file=sys.stderr)
        sys.exit(1)

    # 加载模型（只加载一次）
    print(f"加载模型: {model_path}", file=sys.stderr)
    try:
        voice = PiperVoice.load(model_path)
        print(f"✓ 模型已加载，采样率 {voice.config.sample_rate} Hz", file=sys.stderr)
        sys.stderr.flush()
    except Exception as e:
        print(f"错误: 加载模型失败: {e}", file=sys.stderr)
        sys.exit(1)

    # 循环处理合成请求
    for line in sys.stdin:
        text_file = Path(line.strip())
        if not text_file or not text_file.exists():
            # 发送空响应（长度 0）
            sys.stdout.buffer.write(struct.pack('<I', 0))
            sys.stdout.buffer.flush()
            continue

        try:
            text = text_file.read_text(encoding='utf-8').strip()
            if not text:
                sys.stdout.buffer.write(struct.pack('<I', 0))
                sys.stdout.buffer.flush()
                continue

            # 合成音频
            pcm_chunks = []
            for audio_chunk in voice.synthesize(text):
                pcm_chunks.append(audio_chunk.audio_int16_bytes)

            pcm_bytes = b''.join(pcm_chunks)

            # 先发送长度，再发送数据
            sys.stdout.buffer.write(struct.pack('<I', len(pcm_bytes)))
            sys.stdout.buffer.write(pcm_bytes)
            sys.stdout.buffer.flush()

        except Exception as e:
            print(f"错误: 合成失败: {e}", file=sys.stderr)
            sys.stderr.flush()
            # 发送空响应
            sys.stdout.buffer.write(struct.pack('<I', 0))
            sys.stdout.buffer.flush()

if __name__ == "__main__":
    main()
