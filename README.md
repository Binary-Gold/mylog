# myLog

个人练习用 C++ 小库：**mmap 辅助**、**日志相关头文件**、**utils（如 `space` 容量单位）** 等。目录按模块分，不打算拆成多个仓库。

## 依赖

- C++20 编译器
- [fmt](https://github.com/fmtlib/fmt)：仓库内 `third_party/fmt`，由 CMake `add_subdirectory` 构建
- 测试（可选）：GoogleTest，`MYLOG_BUILD_TESTS=ON` 时通过 `find_package(GTest CONFIG REQUIRED)`，需本机已安装或通过包管理提供 CMake 配置

## 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

开启测试与 `ctest`：

```bash
cmake -S . -B build -DMYLOG_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

Windows 可改用 Visual Studio 生成器或 Ninja，逻辑相同。

## 目录说明

| 路径 | 含义 |
|------|------|
| `include/` | 对外头文件（如 `mmap/`、`logging/`、`utils/`） |
| `src/` | 库实现（`.cpp`），与 `include` 下模块对应 |
| `test/` | 测试与带 `main` 的小程序：见下 |
| `third_party/` | 第三方源码（当前为 fmt） |

**测试约定（CMake）**：`test/`、`tests/` 下名为 `main.cpp` 的文件会单独生成可执行文件；`test_*.cpp` 等由 `mylog_gtest_tests` 汇总，供 GTest 发现。

## 模块依赖（自觉维护）

- `utils`（如 `space`、底层 IO 封装）尽量不依赖 `mmap` / `logging`
- `mmap` 可实现上依赖 `utils`（如 Linux 文件描述符封装），**避免** `logging` ↔ `mmap` 互相包含

## 宏日志（可选）

若编译时定义 `ENABLE_LOG`，`include/logging/internal_log.hpp` 中的 `LOG_INFO` / `LOG_WARN` / `LOG_ERROR` 会通过 fmt 输出。需在链接到 `mylog` 的目标上继续能解析 fmt（本仓库中库已 `PUBLIC` 链接 `fmt::fmt`）。

在 CMake 里示例：

```cmake
target_compile_definitions(my_target PRIVATE ENABLE_LOG)
```
