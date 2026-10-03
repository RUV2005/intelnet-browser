import React, { useState, useEffect, useRef, useLayoutEffect, createContext, useContext } from 'react';
import { createRoot } from 'react-dom/client';
import { gsap } from 'gsap';
import { ScrollTrigger } from 'gsap/ScrollTrigger';
import { ScrollToPlugin } from 'gsap/ScrollToPlugin';
import { marked } from 'marked';
import DOMPurify from 'dompurify';
import translations from './translations.js';
import './styles.css';

gsap.registerPlugin(ScrollTrigger, ScrollToPlugin);

marked.setOptions({
  breaks: true,
  gfm: true
});

function MarkdownContent({ content }) {
  const html = DOMPurify.sanitize(marked.parse(content || ''));
  return <div className="markdown-content" dangerouslySetInnerHTML={{ __html: html }} />;
}

// 语言上下文
const LanguageContext = createContext();

const useLanguage = () => {
  const context = useContext(LanguageContext);
  if (!context) {
    throw new Error('useLanguage must be used within LanguageProvider');
  }
  return context;
};

const workflows = [
  {
    id: 'summary',
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

这一突破为量子计算的实际应用铺平了道路，特别是在药物发现、材料科学和密码学领域。研究团队预计在2026年实现商业化应用。`,
    generateResponse: (input) => {
      return `这是一篇关于量子计算突破的技术报道。文章介绍了新型量子处理器实现了127个量子比特的稳定运行，错误率降低了40%。

主要突破点包括：
1. 量子纠错技术的改进
2. 更长的相干时间（从100微秒提升到200微秒）
3. 在药物发现领域的潜在应用

研究团队预计在2026年实现商业化应用。`;
    }
  },
  {
    id: 'image',
    label: '图像分析',
    title: '看见图像的含义',
    description: '上传图片或粘贴图片链接，AI 描述视觉内容、识别物体。',
    placeholder: '将图片拖到这里，或点击上传...',
    isImageUpload: true,
    generateResponse: (input) => {
      return `这是一张数据可视化图表，展示了2020-2024年全球可再生能源装机容量的增长趋势。

关键信息：
- 太阳能（黄色线）增长最快，从180GW增至420GW
- 风能（蓝色线）稳步增长，从95GW增至210GW
- 水电（绿色线）保持相对稳定
- 图表右下角标注数据来源：国际能源署2024年报告

图表使用了清晰的配色方案，三条趋势线易于区分，整体呈现出可再生能源快速发展的态势。`;
    }
  },
  {
    id: 'voice',
    label: '语音朗读',
    title: '听见网页内容',
    description: '输入任何文本，中文 TTS 引擎将内容转化为自然语音。',
    placeholder: '输入要朗读的文字...',
    demoContent: '量子计算的最新突破标志着我们向实用化量子计算机迈进了重要一步。新型处理器不仅提升了量子比特数量，更重要的是大幅降低了错误率，这是实现复杂量子算法的关键。',
    generateResponse: (input) => {
      return `🔊 正在朗读...

"${input.slice(0, 100)}${input.length > 100 ? '...' : ''}"

✓ 朗读完成

语音参数：
- 语言：中文普通话
- 语速：标准
- 音调：自然
- 引擎：Piper TTS

需要调整语速或重新朗读吗？`;
    }
  }
];

const compressImage = (file) => new Promise((resolve, reject) => {
  const reader = new FileReader();

  reader.onerror = () => reject(new Error('无法读取图片文件'));
  reader.onload = () => {
    const image = new Image();
    image.onerror = () => reject(new Error('无法解析图片文件'));
    image.onload = () => {
      const maxDimension = 1600;
      const scale = Math.min(1, maxDimension / Math.max(image.naturalWidth, image.naturalHeight));
      const width = Math.max(1, Math.round(image.naturalWidth * scale));
      const height = Math.max(1, Math.round(image.naturalHeight * scale));
      const canvas = document.createElement('canvas');
      const context = canvas.getContext('2d');

      if (!context) {
        reject(new Error('当前浏览器不支持图片压缩'));
        return;
      }

      canvas.width = width;
      canvas.height = height;
      context.fillStyle = '#ffffff';
      context.fillRect(0, 0, width, height);
      context.drawImage(image, 0, 0, width, height);
      resolve(canvas.toDataURL('image/jpeg', 0.8));
    };
    image.src = reader.result;
  };

  reader.readAsDataURL(file);
});

function Header() {
  const headerRef = useRef(null);
  const { lang, setLang, t } = useLanguage();

  useLayoutEffect(() => {
    const header = headerRef.current;

    ScrollTrigger.create({
      start: 'top -80',
      end: 99999,
      toggleClass: { targets: header, className: 'scrolled' }
    });

    return () => {
      ScrollTrigger.getAll().forEach(t => t.kill());
    };
  }, []);

  const scrollToSection = (e, target) => {
    e.preventDefault();
    gsap.to(window, {
      duration: 1.2,
      scrollTo: { y: target, offsetY: 80 },
      ease: 'power3.inOut'
    });
  };

  const toggleLanguage = () => {
    setLang(lang === 'zh' ? 'en' : 'zh');
  };

  return (
    <header ref={headerRef} className="header">
      <div className="header-content">
        <div className="logo">{lang === 'zh' ? '明镜浏览器' : 'MingJing Browser'}</div>
        <nav className="nav">
          <a href="#demo" onClick={(e) => scrollToSection(e, '#demo')}>{t.nav.demo}</a>
          <a href="#architecture" onClick={(e) => scrollToSection(e, '#architecture')}>{t.nav.architecture}</a>
          <button className="lang-switcher" onClick={toggleLanguage}>
            {lang === 'zh' ? 'EN' : '中文'}
          </button>
          <a
            href="https://github.com/RUV2005/intelnet-browser"
            className="nav-cta"
            target="_blank"
            rel="noreferrer"
          >
            {t.nav.github}
          </a>
        </nav>
      </div>
    </header>
  );
}

function Hero() {
  const heroRef = useRef(null);
  const titleRef = useRef(null);
  const subtitleRef = useRef(null);
  const visualRef = useRef(null);
  const { t } = useLanguage();

  useLayoutEffect(() => {
    const ctx = gsap.context(() => {
      const tl = gsap.timeline({ defaults: { ease: 'power3.out' } });

      tl.from(titleRef.current, {
        y: 60,
        opacity: 0,
        duration: 1.2,
        delay: 0.3
      })
      .from(subtitleRef.current, {
        y: 40,
        opacity: 0,
        duration: 1,
      }, '-=0.8')
      .from(visualRef.current, {
        y: 80,
        opacity: 0,
        scale: 0.95,
        duration: 1.2,
      }, '-=0.9');

      gsap.to(heroRef.current, {
        scrollTrigger: {
          trigger: heroRef.current,
          start: 'top top',
          end: 'bottom top',
          scrub: 1.5,
        },
        y: 200,
        opacity: 0,
        ease: 'none'
      });

      gsap.to(visualRef.current, {
        scrollTrigger: {
          trigger: heroRef.current,
          start: 'top top',
          end: 'bottom top',
          scrub: 2,
        },
        y: 100,
        scale: 0.9,
        ease: 'none'
      });

    }, heroRef);

    return () => ctx.revert();
  }, []);

  const scrollToDemo = (e) => {
    e.preventDefault();
    gsap.to(window, {
      duration: 1.2,
      scrollTo: { y: '#demo', offsetY: 80 },
      ease: 'power3.inOut'
    });
  };

  return (
    <section ref={heroRef} className="hero">
      <div className="hero-content">
        <h1 ref={titleRef} className="hero-title">
          {t.hero.title1}<br />
          <span className="highlight">{t.hero.title2}</span>
        </h1>
        <p ref={subtitleRef} className="hero-subtitle">
          {t.hero.subtitle}<br />
          {t.hero.subtitle2}
        </p>
        <button className="hero-cta" onClick={scrollToDemo}>
          {t.hero.cta}
        </button>
      </div>
      <div ref={visualRef} className="hero-visual">
        <BrowserFrame />
      </div>
    </section>
  );
}

function BrowserFrame() {
  const frameRef = useRef(null);
  const contentLinesRef = useRef([]);

  useLayoutEffect(() => {
    const ctx = gsap.context(() => {
      contentLinesRef.current.forEach((line, i) => {
        if (line) {
          gsap.to(line, {
            opacity: 0.6,
            duration: 1.5,
            repeat: -1,
            yoyo: true,
            ease: 'sine.inOut',
            delay: i * 0.1
          });
        }
      });

      const frame = frameRef.current;
      frame.addEventListener('mouseenter', () => {
        gsap.to(frame, {
          y: -8,
          scale: 1.01,
          duration: 0.6,
          ease: 'power2.out'
        });
      });

      frame.addEventListener('mouseleave', () => {
        gsap.to(frame, {
          y: 0,
          scale: 1,
          duration: 0.6,
          ease: 'power2.out'
        });
      });

    }, frameRef);

    return () => ctx.revert();
  }, []);

  return (
    <div ref={frameRef} className="browser-frame">
      <div className="browser-chrome">
        <div className="browser-dots">
          <span></span>
          <span></span>
          <span></span>
        </div>
        <div className="browser-address">https://example.com/quantum-computing</div>
      </div>
      <div className="browser-content">
        <div className="content-area">
          <div ref={el => contentLinesRef.current[0] = el} className="content-line long"></div>
          <div ref={el => contentLinesRef.current[1] = el} className="content-line"></div>
          <div ref={el => contentLinesRef.current[2] = el} className="content-line medium"></div>
          <div ref={el => contentLinesRef.current[3] = el} className="content-line"></div>
          <div ref={el => contentLinesRef.current[4] = el} className="content-block"></div>
          <div ref={el => contentLinesRef.current[5] = el} className="content-line"></div>
          <div ref={el => contentLinesRef.current[6] = el} className="content-line long"></div>
        </div>
        <div className="assistant-preview">
          <div className="assistant-header">AI 助手</div>
          <div className="assistant-message">
            <div className="message-label">摘要</div>
            <div className="message-text">这是一篇关于量子计算突破的技术报道...</div>
          </div>
        </div>
      </div>
    </div>
  );
}

function InteractiveDemoSection() {
  const [activeTab, setActiveTab] = useState('summary');
  const [inputValue, setInputValue] = useState('');
  const [response, setResponse] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [isCompressing, setIsCompressing] = useState(false);
  const [uploadedImage, setUploadedImage] = useState(null);

  const sectionRef = useRef(null);
  const demoRef = useRef(null);
  const infoRef = useRef(null);

  const { t } = useLanguage();
  const currentWorkflow = t.demo.workflows[activeTab];

  useLayoutEffect(() => {
    const ctx = gsap.context(() => {
      gsap.from(demoRef.current, {
        scrollTrigger: {
          trigger: sectionRef.current,
          start: 'top 70%',
          end: 'top 30%',
          scrub: 1,
        },
        x: -100,
        opacity: 0,
      });

      gsap.from(infoRef.current, {
        scrollTrigger: {
          trigger: sectionRef.current,
          start: 'top 70%',
          end: 'top 30%',
          scrub: 1,
        },
        x: 100,
        opacity: 0,
      });

    }, sectionRef);

    return () => ctx.revert();
  }, []);

  const handleTabClick = (tabId) => {
    if (tabId !== activeTab) {
      setActiveTab(tabId);
      setInputValue('');
      setResponse('');
      setUploadedImage(null);
    }
  };

  const handleTryDemo = () => {
    if (currentWorkflow.demoContent) {
      setInputValue(currentWorkflow.demoContent);
    }
  };

  const handleSubmit = async () => {
    if (!inputValue.trim() && !uploadedImage) return;

    setIsLoading(true);
    setResponse('');

    try {
      let result;
      const API_BASE = import.meta.env.VITE_API_BASE || '/api';

      if (activeTab === 'summary') {
        const response = await fetch(`${API_BASE}/summarize`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ text: inputValue })
        });
        const data = await response.json();
        if (!response.ok) throw new Error(data.error || `请求失败 (${response.status})`);
        result = data.success ? data.result : `错误: ${data.error}`;
      } else if (activeTab === 'image') {
        const response = await fetch(`${API_BASE}/analyze-image`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ imageData: uploadedImage })
        });
        if (!response.ok) {
          const data = await response.json().catch(() => ({}));
          throw new Error(data.error || `请求失败 (${response.status})`);
        }

        const reader = response.body?.getReader();
        if (!reader) throw new Error('浏览器不支持流式响应');
        const decoder = new TextDecoder();
        let buffer = '';
        let streamedResult = '';

        const consumeEvent = (event) => {
          const dataLine = event.split('\n').find(line => line.startsWith('data: '));
          if (!dataLine) return;
          const payload = JSON.parse(dataLine.slice(6));
          if (payload.error) throw new Error(payload.error);
          if (payload.delta) {
            streamedResult += payload.delta;
            setResponse(streamedResult);
          }
        };

        while (true) {
          const { value, done } = await reader.read();
          buffer += decoder.decode(value || new Uint8Array(), { stream: !done });
          const events = buffer.split('\n\n');
          buffer = events.pop() || '';
          events.forEach(consumeEvent);
          if (done) break;
        }
        if (buffer.trim()) consumeEvent(buffer);
        result = null;
      } else if (activeTab === 'voice') {
        const response = await fetch(`${API_BASE}/text-to-speech`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ text: inputValue })
        });
        const data = await response.json();
        if (!response.ok) throw new Error(data.error || `请求失败 (${response.status})`);
        result = data.success ? data.result : `错误: ${data.error}`;
      }

      if (activeTab !== 'image') setResponse(result);
    } catch (error) {
      setResponse(`连接错误: ${error.message}\n\n请确保后端服务器正在运行 (npm run server)`);
    } finally {
      setIsLoading(false);
    }
  };

  const processImageFile = async (file) => {
    if (!file || !file.type.startsWith('image/')) return;

    setIsCompressing(true);
    setResponse('');
    try {
      const compressedImage = await compressImage(file);
      setUploadedImage(compressedImage);
      setInputValue('');
    } catch (error) {
      setUploadedImage(null);
      setResponse(`图片处理失败: ${error.message}`);
    } finally {
      setIsCompressing(false);
    }
  };

  const handleImageUpload = (e) => {
    void processImageFile(e.target.files?.[0]);
    e.target.value = '';
  };

  const handleDragOver = (e) => {
    e.preventDefault();
  };

  const handleDrop = (e) => {
    e.preventDefault();
    const file = e.dataTransfer.files[0];
    void processImageFile(file);
  };

  return (
    <section ref={sectionRef} className="demo-section" id="demo">
      <div className="demo-container">
        <div ref={infoRef} className="demo-info">
          <h2 className="demo-section-title">{t.demo.title}</h2>
          <p className="demo-section-subtitle">
            {t.demo.subtitle}
          </p>
          <div className="workflow-tabs">
            {['summary', 'image', 'voice'].map(id => (
              <button
                key={id}
                className={`tab ${activeTab === id ? 'active' : ''}`}
                onClick={() => handleTabClick(id)}
              >
                {t.demo.workflows[id].label}
              </button>
            ))}
          </div>
          <h3 className="demo-title">{currentWorkflow.title}</h3>
          <p className="demo-description">{currentWorkflow.description}</p>
          {currentWorkflow.demoContent && (
            <button className="demo-try-btn" onClick={handleTryDemo}>
              {t.demo.loadDemo}
            </button>
          )}
        </div>

        <div ref={demoRef} className="demo-playground">
          <div className="demo-input-area">
            {activeTab === 'image' ? (
              <div
                className="image-upload-area"
                onDragOver={handleDragOver}
                onDrop={handleDrop}
              >
                {uploadedImage ? (
                  <div className="uploaded-image-preview">
                    <img src={uploadedImage} alt="Uploaded" />
                    <button
                      className="remove-image-btn"
                      onClick={() => setUploadedImage(null)}
                    >
                      ✕
                    </button>
                  </div>
                ) : (
                  <>
                    <input
                      type="file"
                      accept="image/*"
                      onChange={handleImageUpload}
                      id="image-upload"
                      style={{ display: 'none' }}
                    />
                    <label htmlFor="image-upload" className="upload-label">
                      <div className="upload-icon">📸</div>
                      <div className="upload-text">{currentWorkflow.placeholder}</div>
                    </label>
                  </>
                )}
              </div>
            ) : (
              <textarea
                className="demo-textarea"
                placeholder={currentWorkflow.placeholder}
                value={inputValue}
                onChange={(e) => setInputValue(e.target.value)}
                rows={8}
              />
            )}

            <button
              className="demo-submit-btn"
              onClick={handleSubmit}
              disabled={(!inputValue.trim() && !uploadedImage) || isLoading || isCompressing}
            >
              {isCompressing ? t.demo.compressing : isLoading ? t.demo.analyzing : t.demo.analyze}
            </button>
          </div>

          {(response || isLoading) && (
            <div className="demo-output-area">
              <div className="output-header">
                <span className="output-label">{t.demo.result}</span>
              </div>
              <div className="output-content">
                {response ? (
                  <MarkdownContent content={response} />
                ) : isLoading ? (
                  <div className="loading-animation">
                    <div className="loading-dot"></div>
                    <div className="loading-dot"></div>
                    <div className="loading-dot"></div>
                  </div>
                ) : null}
              </div>
            </div>
          )}
        </div>
      </div>
    </section>
  );
}

function ArchitectureSection() {
  const { t } = useLanguage();
  const sectionRef = useRef(null);
  const titleRef = useRef(null);
  const layersRef = useRef([]);
  const featuresRef = useRef([]);

  useLayoutEffect(() => {
    const ctx = gsap.context(() => {
      layersRef.current.forEach(layer => {
        if (layer) {
          layer.addEventListener('mouseenter', () => {
            gsap.to(layer, {
              scale: 1.03,
              backgroundColor: 'rgba(247, 247, 248, 0.1)',
              duration: 0.4,
              ease: 'power2.out'
            });
          });

          layer.addEventListener('mouseleave', () => {
            gsap.to(layer, {
              scale: 1,
              backgroundColor: 'rgba(247, 247, 248, 0.05)',
              duration: 0.4,
              ease: 'power2.out'
            });
          });
        }
      });

    }, sectionRef);

    return () => ctx.revert();
  }, []);

  return (
    <section ref={sectionRef} className="architecture-section" id="architecture">
      <div className="architecture-content">
        <div ref={titleRef}>
          <h2 className="section-title">{t.architecture.title}</h2>
          <p className="section-subtitle">
            {t.architecture.subtitle}
          </p>
        </div>
        <div className="stack-diagram">
          <div ref={el => layersRef.current[0] = el} className="stack-layer">
            <div className="layer-label">{t.architecture.layers.qt.label}</div>
            <div className="layer-tech">{t.architecture.layers.qt.tech}</div>
          </div>
          <div className="stack-connector">C FFI</div>
          <div ref={el => layersRef.current[1] = el} className="stack-layer">
            <div className="layer-label">{t.architecture.layers.rust.label}</div>
            <div className="layer-tech">{t.architecture.layers.rust.tech}</div>
          </div>
        </div>
        <div className="architecture-features">
          <div ref={el => featuresRef.current[0] = el} className="feature">
            <h3>{t.architecture.features.f1.title}</h3>
            <p>{t.architecture.features.f1.desc}</p>
          </div>
          <div ref={el => featuresRef.current[1] = el} className="feature">
            <h3>{t.architecture.features.f2.title}</h3>
            <p>{t.architecture.features.f2.desc}</p>
          </div>
          <div ref={el => featuresRef.current[2] = el} className="feature">
            <h3>{t.architecture.features.f3.title}</h3>
            <p>{t.architecture.features.f3.desc}</p>
          </div>
        </div>
      </div>
    </section>
  );
}

function CTASection() {
  const { t } = useLanguage();
  const sectionRef = useRef(null);
  const contentRef = useRef(null);
  const buttonsRef = useRef([]);

  useLayoutEffect(() => {
    const ctx = gsap.context(() => {
      buttonsRef.current.forEach(btn => {
        if (btn) {
          btn.addEventListener('mouseenter', () => {
            gsap.to(btn, {
              y: -4,
              scale: 1.02,
              duration: 0.4,
              ease: 'power2.out'
            });
          });

          btn.addEventListener('mouseleave', () => {
            gsap.to(btn, {
              y: 0,
              scale: 1,
              duration: 0.4,
              ease: 'power2.out'
            });
          });
        }
      });

    }, sectionRef);

    return () => ctx.revert();
  }, []);

  return (
    <section ref={sectionRef} className="cta-section">
      <div ref={contentRef} className="cta-content">
        <h2 className="cta-title">{t.cta.title}</h2>
        <p className="cta-subtitle">
          {t.cta.subtitle}
        </p>
        <div className="cta-buttons">
          <a
            ref={el => buttonsRef.current[0] = el}
            href="https://github.com/RUV2005/intelnet-browser"
            className="btn btn-primary"
            id="github"
          >
            {t.cta.github}
          </a>
          <a
            ref={el => buttonsRef.current[1] = el}
            href="https://github.com/RUV2005/intelnet-browser/blob/main/BUILD_GUIDE.md"
            className="btn btn-secondary"
          >
            {t.cta.buildGuide}
          </a>
        </div>
      </div>
    </section>
  );
}

function Footer() {
  const { t } = useLanguage();
  const footerRef = useRef(null);
  const linksRef = useRef([]);

  useLayoutEffect(() => {
    const ctx = gsap.context(() => {
      linksRef.current.forEach(link => {
        if (link) {
          link.addEventListener('mouseenter', () => {
            gsap.to(link, {
              x: 6,
              duration: 0.3,
              ease: 'power2.out'
            });
          });

          link.addEventListener('mouseleave', () => {
            gsap.to(link, {
              x: 0,
              duration: 0.3,
              ease: 'power2.out'
            });
          });
        }
      });
    }, footerRef);

    return () => ctx.revert();
  }, []);

  return (
    <footer ref={footerRef} className="footer">
      <div className="footer-content">
        <div className="footer-brand">
          <div className="footer-logo">{t.footer.brand}</div>
          <p className="footer-tagline">{t.footer.tagline}</p>
        </div>
        <div className="footer-links">
          <div className="footer-column">
            <h4>{t.footer.product.title}</h4>
            <a ref={el => linksRef.current[0] = el} href="#demo">{t.footer.product.demo}</a>
            <a ref={el => linksRef.current[1] = el} href="#architecture">{t.footer.product.architecture}</a>
            <a ref={el => linksRef.current[2] = el} href="https://github.com/RUV2005/intelnet-browser/blob/main/CHANGELOG.md">{t.footer.product.changelog}</a>
          </div>
          <div className="footer-column">
            <h4>{t.footer.developers.title}</h4>
            <a ref={el => linksRef.current[3] = el} href="https://github.com/RUV2005/intelnet-browser">{t.footer.developers.github}</a>
            <a ref={el => linksRef.current[4] = el} href="https://github.com/RUV2005/intelnet-browser/blob/main/BUILD_GUIDE.md">{t.footer.developers.buildGuide}</a>
            <a ref={el => linksRef.current[5] = el} href="https://github.com/RUV2005/intelnet-browser/blob/main/CONTRIBUTING.md">{t.footer.developers.contributing}</a>
          </div>
          <div className="footer-column">
            <h4>{t.footer.resources.title}</h4>
            <a ref={el => linksRef.current[6] = el} href="https://github.com/RUV2005/intelnet-browser/blob/main/README.md">{t.footer.resources.docs}</a>
            <a ref={el => linksRef.current[7] = el} href="https://github.com/RUV2005/intelnet-browser/issues">{t.footer.resources.issues}</a>
          </div>
        </div>
      </div>
      <div className="footer-bottom">
        <p>{t.footer.copyright}</p>
      </div>
    </footer>
  );
}

function App() {
  const [lang, setLang] = useState('zh');
  const t = translations[lang];

  useEffect(() => {
    gsap.config({
      force3D: true,
      nullTargetWarn: false,
    });

    const cursor = document.createElement('div');
    cursor.className = 'custom-cursor';
    document.body.appendChild(cursor);

    const moveCursor = (e) => {
      gsap.to(cursor, {
        x: e.clientX,
        y: e.clientY,
        duration: 0.3,
        ease: 'power2.out'
      });
    };

    window.addEventListener('mousemove', moveCursor);

    return () => {
      window.removeEventListener('mousemove', moveCursor);
      if (cursor.parentNode) {
        cursor.parentNode.removeChild(cursor);
      }
    };
  }, []);

  return (
    <LanguageContext.Provider value={{ lang, setLang, t }}>
      <div className="app">
        <Header />
        <Hero />
        <InteractiveDemoSection />
        <ArchitectureSection />
        <CTASection />
        <Footer />
      </div>
    </LanguageContext.Provider>
  );
}

createRoot(document.getElementById('root')).render(<App />);
