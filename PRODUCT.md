# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

## Stack

React/Vite for an independent marketing website in `website/`, alongside the existing Qt + Rust desktop application.

## Users

The website serves both everyday users who need accessible browsing, AI page understanding, or voice assistance, and developers who want to understand, build, or contribute to IntelNet Browser.

## Product Purpose

IntelNet Browser is an open-source accessibility-focused browser that combines a Qt WebEngine client with Rust-powered AI and text-to-speech capabilities. The website should help visitors understand the product quickly, see its core workflow in action, and find a path to download, build, or contribute.

## Positioning

IntelNet connects browsing, page understanding, image analysis, and spoken assistance in one native browser experience instead of treating accessibility as a separate utility layered on top of the browser.

## Operating Context

Users browse the web on Windows through the Qt desktop client. The project is built with Qt 6, CMake, Rust, and a C FFI bridge. Developers use the repository documentation, Cargo, CMake, and Visual Studio to build and test the project.

## Capabilities and Constraints

- Qt 6 WebEngine browser client.
- Rust AI and TTS core exposed through C FFI.
- DOM-based page summaries, web image selection and analysis, and Piper Chinese TTS are implemented or integrated.
- Accessibility features include keyboard shortcuts, focus indicators, and high contrast support.
- The website is a new React/Vite surface and should not alter the native client architecture.
- Download packaging and live product metrics are not confirmed; the website must not invent release links, customers, benchmarks, or pricing.

## Brand Commitments

The product name is IntelNet Browser. It is open source and currently documented in Chinese and English. The project should communicate clearly and respectfully, with accessibility treated as a first-class product concern rather than a marketing afterthought.

## Evidence on Hand

- `BUILD_GUIDE.md` documents the Qt + Rust architecture and build process.
- `CHANGELOG.md` documents the 0.1.0 feature set and current project history.
- `qt-client/` contains the native browser client and UI resources.
- `rust-core/` contains the AI/TTS core.
- No verified production screenshots, release download URLs, customer testimonials, or benchmark data are available in the repository.

## Product Principles

- Make the web easier to perceive, understand, and hear.
- Prove the workflow instead of hiding it behind claims.
- Keep accessibility visible in the product's main path.
- Give developers enough technical truth to build trust and contribute.
- Never fabricate evidence the project does not yet have.
