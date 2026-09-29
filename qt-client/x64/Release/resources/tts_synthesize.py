#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TTS Synthesis Script for IntelNet Browser
Uses edge-tts for text-to-speech synthesis
"""

import sys
import asyncio
import edge_tts
import os

async def synthesize(text: str, output_file: str, voice: str = "zh-CN-XiaoxiaoNeural"):
    """Synthesize speech from text using edge-tts"""
    try:
        communicate = edge_tts.Communicate(text, voice)
        await communicate.save(output_file)
        print(f"SUCCESS: Audio saved to {output_file}", file=sys.stderr)
        return 0
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 1

def main():
    if len(sys.argv) < 3:
        print("Usage: tts_synthesize.py <text> <output_file> [voice]", file=sys.stderr)
        sys.exit(1)

    text = sys.argv[1]
    output_file = sys.argv[2]
    voice = sys.argv[3] if len(sys.argv) > 3 else "zh-CN-XiaoxiaoNeural"

    result = asyncio.run(synthesize(text, output_file, voice))
    sys.exit(result)

if __name__ == "__main__":
    main()
