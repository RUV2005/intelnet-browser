// Rust 核心性能基准测试
// 使用 criterion 框架进行性能测试

use criterion::{black_box, criterion_group, criterion_main, Criterion};
use std::ffi::CString;

// 注意：这些是 FFI 函数的基准测试
// 实际的 AI 推理性能取决于硬件和模型

extern "C" {
    fn intelnet_init() -> i32;
    fn intelnet_shutdown();
    fn intelnet_free_string(ptr: *mut std::os::raw::c_char);
}

fn bench_init_shutdown(c: &mut Criterion) {
    c.bench_function("init_shutdown", |b| {
        b.iter(|| unsafe {
            intelnet_init();
            intelnet_shutdown();
        })
    });
}

fn bench_string_allocation(c: &mut Criterion) {
    c.bench_function("string_alloc_free", |b| {
        b.iter(|| {
            let test_str = CString::new("test string for benchmarking").unwrap();
            let raw_ptr = test_str.into_raw();
            unsafe {
                intelnet_free_string(raw_ptr);
            }
        })
    });
}

criterion_group!(benches, bench_init_shutdown, bench_string_allocation);
criterion_main!(benches);
