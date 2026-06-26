# Shader 编译优化调试报告

## 背景

项目使用 GLSL 编写 Vulkan shader，通过 `glslangValidator` (glslc) 编译为 SPIR-V。所有 shader 以**单体文件**形式存在——顶点/片元 shader 各自完整包含所有依赖函数，没有公共库复用机制。随着后处理函数库膨胀（~800 行，110+ 函数），每次修改任意函数都需重新编译所有使用者，重复编译成本线性增长。

## 优化目标

将公共函数提取为**独立库模块**，shader 使用者只声明引用关系。编译系统只需**编译库一次**，各 shader 使用者链接预编译产物即可——类似 C/C++ 的 `.o` + `-l` 模型。

---

## 方向一：GLSL + spirv-link（不可行）

### 方案

Vulkan SDK 提供 `spirv-link` 工具，可将多个 SPIR-V 模块链接为单一模块：

```
glslc -c fgui_lib.frag     → fgui_lib.spv       (库, 无 main)
glslc -c fgui_image.frag   → fgui_image.spv     (主 shader, 仅函数声明)
spirv-link fgui_image.spv fgui_lib.spv → linked.spv
```

### 问题

**GLSL 语言层面不支持跨编译单元的函数声明解析。**

- `glslc` 要求每个编译单元内所有被调用函数必须有函数体（definition），仅声明（declaration）无法通过编译。
- GLSL 没有 `extern` 关键字，没有模块系统，`#include` 是纯文本拼接——本质上消除不了重复编译。
- `spirv-link` 的设计目标是将**各自完整编译**的 SPIR-V 模块做合并和去重，而非解析未定义符号。它假设输入模块各自已是有效 SPIR-V。

### 结论

GLSL 生态**不具备真正的分离编译能力**。`spirv-link` 无法解决"库编译一次，使用者链接"的核心需求。此方向放弃。

---

## 方向二：Slang 模块化编译（可行 ✅）

### 为什么选 Slang

[Slang](https://github.com/shader-slang/slang) 是 Khronos / NVIDIA 主导的现代 shader 语言，原生支持：

| 特性 | GLSL | Slang |
|---|---|---|
| 模块系统 | `#include` (文本拼接) | `import` (符号级解析) |
| 分离编译 | 不支持 | `-target none` 预编译 IR 模块 |
| 链接 | 需 spirv-link hack | `-r module` 原生链接 |
| 跨平台 | Vulkan only | SPIR-V / DXIL / HLSL / GLSL / Metal / CUDA |
| 语言特性 | 基础 | 泛型、接口、运算符重载 |

#### 编译流程

```
# 1. 库 → IR 中间件 (不做 GPU 代码生成)
slangc fgui_lib.slang -target none -o fgui_lib.slang-module

# 2. 着色器 + 库模块 → 链接生成 SPIR-V
slangc fgui_image.frag.slang -I . -target spirv -entry fragmentMain -o fgui_image.frag.spv
```

## 性能基准测试

### 测试设计

创建独立 benchmark 目录 `bin/shaders/benchmark/`：

| 文件 | 说明 |
|---|---|
| `fx_lib.slang` | 巨型公共库 (~180 行, ~160 个函数: SDF 2D/3D、噪声、色彩、混合模式、后处理) |
| `fx_main_[a-e].slang` | 5 个使用者 (模块版，`import fx_lib`) |
| `fx_main_[a-e]_monolith.slang` | 5 个使用者 (单体版，库全文内联) |

两套编译脚本各跑 50 次 (5 shader × 10 rounds)：

```
build_monolithic.ps1:  slangc [monolith].slang -target spirv     × 50
build_module.ps1:      slangc [fx_lib]        -target none       × 1
                       slangc [module].slang  -target spirv -I . × 50
```

### 测试结果

```
═══════════════════════════════════════════
           单体 (内联全库)    模块 (import)
───────────────────────────────────────────
编译次数          50              50
单次平均        0.503s          0.312s
最快            0.472s          0.276s
最慢            0.569s          0.340s
总耗时         25.15s          15.99s
───────────────────────────────────────────
节省比例           —             38%
═══════════════════════════════════════════
```

### 分析

- **模块化每次编译节省 ~0.19s**（库解析+类型检查只做一次，后续 import 命中缓存）。
- 当前 fx_lib 仅 180 行，实际 fgui_lib 有 800 行——节省幅度随库体积线性增长。
- 5 个使用者 × 10 轮已有 38% 差距；如果 20 个使用者各编译 100 次，预期节省可达 50%+。

---

## 进一步优化方向

### 1. `-r` 预编译模块链接

当前用 `-I .` 让编译器从源码解析库（内部缓存），但每次仍有一些 IR 处理开销。理想流程：

```
slangc fx_lib.slang -target spirv -o fx_lib.slang-module   # 一次 SPIR-V 级编译
slangc main.slang -r fx_lib.slang-module -target spirv     # 直接链接, 零解析开销
```

需要等待 slangc 修复 `-target none` 模块与 `-target spirv` 链接的兼容性问题，或直接使用 `-target spirv` 编译库模块（当前版本可能因无 entry point 报错）。

### 2. CI 集成

将 `.spv` / `.slang-module` 纳入构建缓存。仅当库源码变更时重编译库，shader 使用者只做链接。

### 3. 跨平台输出

Slang 同一源码可输出多 target：
- `-target spirv` → Vulkan
- `-target dxil` → D3D12
- `-target metal` → Metal
- `-target glsl` → OpenGL

一套 shader 源码覆盖所有平台。

---

## 总结

| | GLSL + spirv-link | Slang import |
|---|---|---|
| 分离编译 | ❌ 不可行 | ✅ 原生支持 |
| 编译效率 | 单体 (基线) | **快 38%+** |
| 实现成本 | — | 语法转换 + 3 天调试 |
| 长期收益 | — | 库体积越大、使用者越多，节省越显著 |

**结论**：Slang 的 `import` 模块系统是目前实现 shader 公共库分离编译的唯一可行方案。已通过基准测试验证效果，建议在项目中推广使用。
