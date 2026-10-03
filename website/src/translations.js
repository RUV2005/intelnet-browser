// 语言配置
const translations = {
  zh: {
    nav: {
      demo: '在线试用',
      architecture: '架构',
      github: 'GitHub'
    },
    hero: {
      title1: '让网页更容易被',
      title2: '看见、理解和听见',
      subtitle: '明镜浏览器将浏览、AI 理解、图像分析和语音朗读融为一体，',
      subtitle2: '让无障碍访问成为浏览器的核心能力。',
      cta: '立即试用 →'
    },
    demo: {
      title: '在线试用',
      subtitle: '立即体验明镜浏览器的核心 AI 功能，无需下载安装。',
      workflows: {
        summary: {
          label: '页面摘要',
          title: '理解整个页面',
          description: '输入任何网址或粘贴文本，AI 即时生成结构化摘要。',
          placeholder: '粘贴文章内容或输入网址...',
          demoContent: `量子计算取得重大突破

研究团队今天宣布，他们成功开发出新型量子处理器，实现了127个量子比特的稳定运行。

关键技术进展：
1. 量子纠错技术得到显著改进
2. 量子比特相干时间从100微秒提升到200微秒
3. 错误率降低了40%

这一突破为量子计算的实际应用铺平了道路，特别是在药物发现、材料科学和密码学领域。研究团队预计在2026年实现商业化应用。`
        },
        image: {
          label: '图像分析',
          title: '看见图像的含义',
          description: '上传图片或粘贴图片链接，AI 描述视觉内容、识别物体。',
          placeholder: '将图片拖到这里，或点击上传...'
        },
        voice: {
          label: '语音朗读',
          title: '听见网页内容',
          description: '输入任何文本，中文 TTS 引擎将内容转化为自然语音。',
          placeholder: '输入要朗读的文字...',
          demoContent: '量子计算的最新突破标志着我们向实用化量子计算机迈进了重要一步。新型处理器不仅提升了量子比特数量，更重要的是大幅降低了错误率，这是实现复杂量子算法的关键。'
        }
      },
      loadDemo: '载入示例内容',
      analyze: '开始分析',
      compressing: '处理图片...',
      analyzing: '处理中...',
      result: 'AI 分析结果'
    },
    architecture: {
      title: '开放的技术架构',
      subtitle: 'Qt WebEngine 前端 + Rust AI 核心，通过 C FFI 桥接，让开发者能够理解、构建和贡献代码。',
      layers: {
        qt: {
          label: 'Qt 6 客户端',
          tech: 'WebEngine · 快捷键 · 高对比度支持'
        },
        rust: {
          label: 'Rust 核心',
          tech: 'AI 分析 · 图像理解 · Piper TTS'
        }
      },
      features: {
        f1: {
          title: 'DOM 页面摘要',
          desc: '基于结构化内容提取的页面理解'
        },
        f2: {
          title: '图像选择分析',
          desc: '网页图片的视觉内容识别与描述'
        },
        f3: {
          title: '中文语音合成',
          desc: '集成 Piper 引擎的自然 TTS'
        }
      }
    },
    cta: {
      title: '开始使用明镜',
      subtitle: '开源项目，完整的构建文档和开发指南',
      github: '访问 GitHub 仓库',
      buildGuide: '查看构建指南'
    },
    footer: {
      brand: '明镜浏览器',
      tagline: '让无障碍成为浏览器的核心能力',
      product: {
        title: '产品',
        demo: '在线试用',
        architecture: '技术架构',
        changelog: '更新日志'
      },
      developers: {
        title: '开发者',
        github: 'GitHub',
        buildGuide: '构建指南',
        contributing: '贡献指南'
      },
      resources: {
        title: '资源',
        docs: '文档',
        issues: '问题反馈'
      },
      copyright: '© 2024 明镜浏览器. 开源项目，遵循 MIT 协议。'
    }
  },
  en: {
    nav: {
      demo: 'Try Online',
      architecture: 'Architecture',
      github: 'GitHub'
    },
    hero: {
      title1: 'Making the web easier to',
      title2: 'see, understand, and hear',
      subtitle: 'MingJing Browser integrates browsing, AI understanding, image analysis,',
      subtitle2: 'and text-to-speech to make accessibility a core capability.',
      cta: 'Try Now →'
    },
    demo: {
      title: 'Try Online',
      subtitle: 'Experience MingJing Browser\'s core AI features instantly, no download required.',
      workflows: {
        summary: {
          label: 'Page Summary',
          title: 'Understand the Entire Page',
          description: 'Enter any URL or paste text, AI instantly generates structured summaries.',
          placeholder: 'Paste article content or enter URL...',
          demoContent: `Quantum Computing Breakthrough

Research team announced today that they have successfully developed a new quantum processor achieving stable operation of 127 qubits.

Key technical advances:
1. Significant improvement in quantum error correction
2. Qubit coherence time increased from 100μs to 200μs
3. Error rate reduced by 40%

This breakthrough paves the way for practical applications of quantum computing, especially in drug discovery, materials science, and cryptography. The team expects commercialization by 2026.`
        },
        image: {
          label: 'Image Analysis',
          title: 'See Image Meaning',
          description: 'Upload images or paste links, AI describes visual content and identifies objects.',
          placeholder: 'Drag image here, or click to upload...'
        },
        voice: {
          label: 'Text-to-Speech',
          title: 'Hear Web Content',
          description: 'Enter any text, TTS engine converts it to natural speech.',
          placeholder: 'Enter text to read aloud...',
          demoContent: 'The latest breakthrough in quantum computing marks an important step toward practical quantum computers. The new processor not only increases the number of qubits but also significantly reduces error rates, which is key to implementing complex quantum algorithms.'
        }
      },
      loadDemo: 'Load Example',
      analyze: 'Start Analysis',
      compressing: 'Processing Image...',
      analyzing: 'Processing...',
      result: 'AI Analysis Result'
    },
    architecture: {
      title: 'Open Architecture',
      subtitle: 'Qt WebEngine frontend + Rust AI core, bridged via C FFI, enabling developers to understand, build, and contribute.',
      layers: {
        qt: {
          label: 'Qt 6 Client',
          tech: 'WebEngine · Shortcuts · High Contrast'
        },
        rust: {
          label: 'Rust Core',
          tech: 'AI Analysis · Image Vision · Piper TTS'
        }
      },
      features: {
        f1: {
          title: 'DOM Page Summary',
          desc: 'Page understanding based on structured content extraction'
        },
        f2: {
          title: 'Image Analysis',
          desc: 'Visual content recognition and description for web images'
        },
        f3: {
          title: 'Text-to-Speech',
          desc: 'Natural TTS with integrated Piper engine'
        }
      }
    },
    cta: {
      title: 'Get Started with MingJing',
      subtitle: 'Open source project with complete build documentation and guides',
      github: 'Visit GitHub Repository',
      buildGuide: 'View Build Guide'
    },
    footer: {
      brand: 'MingJing Browser',
      tagline: 'Making accessibility a core browser capability',
      product: {
        title: 'Product',
        demo: 'Try Online',
        architecture: 'Architecture',
        changelog: 'Changelog'
      },
      developers: {
        title: 'Developers',
        github: 'GitHub',
        buildGuide: 'Build Guide',
        contributing: 'Contributing'
      },
      resources: {
        title: 'Resources',
        docs: 'Documentation',
        issues: 'Report Issues'
      },
      copyright: '© 2024 MingJing Browser. Open source project under MIT License.'
    }
  }
};

export default translations;
