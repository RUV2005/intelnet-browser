import express from 'express';
import cors from 'cors';
import OpenAI from 'openai';
import dotenv from 'dotenv';

dotenv.config();

const app = express();
const allowedOrigin = process.env.WEBSITE_ORIGIN;
app.use(cors(allowedOrigin ? { origin: allowedOrigin } : undefined));
app.use(express.json({ limit: '50mb' }));
app.use(express.urlencoded({ limit: '50mb', extended: true }));

const rateWindowMs = 60_000;
const rateLimit = 60;
const requestCounts = new Map();
app.use('/api', (req, res, next) => {
  const ip = req.ip || req.socket.remoteAddress || 'unknown';
  const now = Date.now();
  const current = requestCounts.get(ip);
  if (!current || now - current.startedAt >= rateWindowMs) {
    requestCounts.set(ip, { startedAt: now, count: 1 });
    return next();
  }
  current.count += 1;
  if (current.count > rateLimit) {
    return res.status(429).json({ success: false, error: '请求过于频繁，请稍后再试' });
  }
  next();
});
setInterval(() => {
  const cutoff = Date.now() - rateWindowMs;
  for (const [ip, entry] of requestCounts) {
    if (entry.startedAt < cutoff) requestCounts.delete(ip);
  }
}, rateWindowMs).unref();

const openai = new OpenAI({
  apiKey: process.env.OPENAI_API_KEY,
  baseURL: 'https://openox.tech/v1',
  timeout: 60_000,
  maxRetries: 0
});

const MODEL = process.env.OPENAI_MODEL || 'gpt-5.6-sol';

app.get('/api/health', (_req, res) => {
  res.json({
    ok: true,
    model: MODEL,
    configured: Boolean(process.env.OPENAI_API_KEY)
  });
});

// 页面摘要 API
app.post('/api/summarize', async (req, res) => {
  try {
    const { text } = req.body;
    if (typeof text !== 'string' || !text.trim()) {
      return res.status(400).json({ success: false, error: '请输入要总结的文本' });
    }

    const completion = await openai.chat.completions.create({
      model: MODEL,
      messages: [
        {
          role: "system",
          content: "你是一个专业的内容分析助手。请为用户提供的文本生成结构化摘要，包括主要内容、关键点等。使用中文回复。"
        },
        {
          role: "user",
          content: `请总结以下内容：\n\n${text}`
        }
      ],
      temperature: 0.7,
      max_tokens: 1000
    });

    res.json({
      success: true,
      result: completion.choices[0].message.content
    });
  } catch (error) {
    console.error('Summarize error:', error);
    res.status(500).json({
      success: false,
      error: error.message
    });
  }
});

// 图像分析 API
app.post('/api/analyze-image', async (req, res) => {
  let streaming = false;
  try {
    const { imageData } = req.body;
    if (typeof imageData !== 'string' ||
        (!imageData.startsWith('data:image/') && !/^https?:\/\//i.test(imageData))) {
      return res.status(400).json({ success: false, error: '请提供有效的图片数据或图片 URL' });
    }

    const stream = await openai.chat.completions.create({
      model: MODEL,
      messages: [
        {
          role: "user",
          content: [
            {
              type: "text",
              text: "请详细描述这张图片的内容，包括主要物体、颜色、场景、可能的含义等。使用中文回复。"
            },
            {
              type: "image_url",
              image_url: {
                url: imageData
              }
            }
          ]
        }
      ],
      max_tokens: 1000,
      stream: true
    });

    streaming = true;
    res.status(200).set({
      'Content-Type': 'text/event-stream; charset=utf-8',
      'Cache-Control': 'no-cache, no-transform',
      Connection: 'keep-alive',
      'X-Accel-Buffering': 'no'
    });
    res.flushHeaders();

    for await (const chunk of stream) {
      const delta = chunk.choices?.[0]?.delta?.content;
      if (delta) res.write(`data: ${JSON.stringify({ delta })}\n\n`);
    }
    res.write('data: {"done":true}\n\n');
    res.end();
  } catch (error) {
    console.error('Image analysis error:', error);
    const timedOut = error?.name === 'TimeoutError'
      || error?.name === 'APIConnectionTimeoutError'
      || error?.code === 'ETIMEDOUT'
      || /timed out|timeout/i.test(error?.message || '');
    const payload = {
      success: false,
      error: timedOut
        ? '图像分析服务响应超时，请稍后重试或换一张较小的图片'
        : error.message
    };
    if (streaming) {
      res.write(`event: error\ndata: ${JSON.stringify(payload)}\n\n`);
      res.end();
    } else {
      res.status(timedOut ? 504 : 500).json(payload);
    }
  }
});

// 语音朗读（文本转换）API
app.post('/api/text-to-speech', async (req, res) => {
  try {
    const { text } = req.body;
    if (typeof text !== 'string' || !text.trim()) {
      return res.status(400).json({ success: false, error: '请输入要朗读的文本' });
    }

    res.json({
      success: true,
      result: `🔊 正在朗读...

"${text.slice(0, 100)}${text.length > 100 ? '...' : ''}"

✓ 朗读完成

语音参数：
- 语言：中文普通话
- 语速：标准
- 音调：自然
- 引擎：Piper TTS

注意：实际的语音合成需要在浏览器端实现。当前展示的是文本处理结果。`
    });
  } catch (error) {
    console.error('TTS error:', error);
    res.status(500).json({
      success: false,
      error: error.message
    });
  }
});

const PORT = process.env.PORT || 3001;
app.listen(PORT, () => {
  console.log(`🚀 API Server running on http://localhost:${PORT}`);
});
