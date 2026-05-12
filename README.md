# mylog

C++20 日志库：控制台输出、加密压缩持久化、mmap 缓冲、异步文件管理。

- `LogHandle` 统一入口，可挂多个 `LogSink`
- `ConsoleSink` 控制台格式化输出
- `EffectiveSink` 加密 + 压缩 + Protobuf 序列化落盘，崩溃恢复
- `Context/Executor` 异步任务框架（线程池 + 定时器）

## 依赖

| 库 | 用途 | 集成方式 |
|---|---|---|
| [fmt](https://github.com/fmtlib/fmt) | 格式化字符串 | `third_party/fmt` |
| [zlib](https://www.zlib.net/) | 压缩 / 解压 | `third_party/zlib` |
| [zstd](https://github.com/facebook/zstd) | 高性能压缩（默认） | `third_party/zstd` |
| [Crypto++](https://www.cryptopp.com/) | ECDH 密钥协商 + AES-128-CBC | `third_party/cryptopp` |
| [Protobuf](https://github.com/protocolbuffers/protobuf) | 日志结构化序列化 | `third_party/protobuf` |
| [GoogleTest](http://google.github.io/googletest/) | 单元测试（可选） | 系统安装 |

C++20 编译器（GCC 11+ / Clang 14+），CMake >= 3.20。

首次使用需拉取 Crypto++：

```bash
git clone https://github.com/weidai11/cryptopp.git third_party/cryptopp
```

## 构建

```bash
# 配置
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 编译（建议控制并行数，abseil/protobuf 编译吃内存）
cmake --build build -j4

# 运行测试
ctest --test-dir build
```

常用选项：

| 选项 | 默认 | 说明 |
|---|---|---|
| `MYLOG_BUILD_TESTS` | ON | 构建测试 |
| `MYLOG_USE_CRYPTOPP` | ON | 启用加密模块 |
| `MYLOG_USE_PROTOBUF` | ON | 启用 Protobuf 序列化 |
| `CMAKE_BUILD_TYPE` | Debug | Release / Debug |
| `CMAKE_INSTALL_PREFIX` | /usr/local | 安装路径 |

关闭可选模块：

```bash
cmake -S . -B build -DMYLOG_USE_CRYPTOPP=OFF -DMYLOG_USE_PROTOBUF=OFF
```

## 安装

```bash
# 安装到自定义路径（推荐）
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/mylog -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
sudo cmake --install build
```

安装产物：

```
/usr/mylog/
  lib/libmylog.a       # 自包含静态库（已合并所有依赖）
  include/logger/       # 头文件
  bin/log_parser        # 日志解析工具
  bin/test_roundtrip    # 往返测试工具
```

`libmylog.a` 是合并了 fmt、zlib、zstd、Crypto++、Protobuf 的完整静态库，你的项目只需链接这一个 `.a`。

## 使用

在你的 CMakeLists.txt 中：

```cmake
target_include_directories(your_target PRIVATE /usr/mylog/include)
target_link_libraries(your_target PRIVATE /usr/mylog/lib/libmylog.a)
```

或直接用命令行：

```bash
g++ -std=c++20 your_code.cpp -I/usr/mylog/include /usr/mylog/lib/libmylog.a
```

## 模块概览

### 核心日志

`LogHandle` 是入口，持有一组 `LogSink`，按级别过滤：

```cpp
auto sink = std::make_shared<logger::ConsoleSink>();
logger::LogHandle log(sink);
log.SetLevel(logger::LogLevel::kInfo);
log.Log(logger::LogLevel::kInfo, CURRENT_SRC(), "hello world");
```

`ConsoleSink` 输出到 stdout，`DefaultFormatter` 生成 `[时间] [级别] [文件:行] 消息` 格式。

### EffectiveSink — 加密持久化

将日志加密、压缩后写入磁盘，支持 crash 恢复：

```cpp
logger::EffectiveSink::Config cfg;
cfg.dir = "/var/log/myapp";
cfg.prefix = "app";
cfg.pub_key = server_pub_key_hex;            // 解析端的 ECDH 公钥
cfg.single_size = logger::megabytes(4);       // 单文件大小上限
cfg.total_size  = logger::megabytes(100);     // 目录总大小上限

auto sink = std::make_shared<logger::EffectiveSink>(cfg);
```

数据流：`LogMsg → Protobuf序列化 → Zstd压缩 → AES-128-CBC加密 → mmap缓冲 → 文件`

### 加密 (crypt)

ECDH 密钥协商 + AES-128-CBC：

```cpp
auto [client_pri, client_pub] = logger::crypt::GenECDHKey();
auto shared = logger::crypt::GenECDHSharedSecret(client_pri, server_pub);
auto aes = logger::crypt::AESCrypt(shared_hex);
```

### 压缩 (compress)

Zlib 和 Zstd 两种实现，`EffectiveSink` 默认使用 Zstd：

```cpp
logger::compress::ZstdCompress comp;
auto compressed = comp.Decompress(data, size);
```

### mmap 缓冲 (mmap)

mmap 双缓冲，用于 `EffectiveSink` 的 crash-safe 写入：

```cpp
logger::mmap::MMapAux buffer;
buffer.Push(data, len);
if (buffer.Ratio() > 0.8) { /* 超过 80% 触发落盘 */ }
```

### 异步框架 (context)

单例 `Context` 管理线程池和定时器，`EffectiveSink` 用它做异步落盘和定期文件清理：

```cpp
auto tag = NEW_TASK_RUNNER("my_task");
POST_TASK(tag, []{ /* 异步执行 */ });
POST_REPEATED_TASK(tag, []{ /* 定时任务 */ }, std::chrono::seconds(5), -1);
```

### 容量单位 (space)

编译期类型安全的文件大小单位（1024 进制）：

```cpp
auto s = logger::megabytes(256);
auto b = logger::space_cast<logger::bytes>(s);  // 268435456
```

## 工具

### log_parser

解密并打印 `EffectiveSink` 生成的 `.log` 文件：

```bash
log_parser /path/to/app_20260512120000.log <server_priv_key_hex>
```

输出带颜色标注的日志行，每行显示时间、级别、进程/线程 ID、文件位置和消息内容。

### test_roundtrip

端到端验证：写入加密日志 → 落盘 → 解密读取 → 比对内容一致。

## 目录结构

```
mylog/
  include/logger/     # 公开头文件
  src/logger/          # 库实现
    compress/          # 压缩（zlib / zstd）
    crypt/             # 加密（ECDH / AES）
    mmap/              # mmap 缓冲
    proto/             # Protobuf 定义与格式化
    context/           # 异步任务框架
  third_party/         # 第三方源码
  test/                # 测试
    unit/              # 单元测试（GoogleTest）
    manual/            # 手动测试 / 性能测试
  tools/               # 命令行工具
  cmake/               # 构建辅助脚本
```

## 日志格式

`EffectiveSink` 落盘文件使用二进制分帧格式，每个 chunk 以 64 字节的 `ChunkHeader`（含 magic、大小、客户端 ECDH 公钥）开头，后面跟多个 `ItemHeader + 加密数据` 块。只有持有服务端私钥才能解密——私钥不落地，仅由 `log_parser` 在读取时提供。

## 测试

```bash
# 全部测试
ctest --test-dir build

# 单独运行某个
./build/test/ut_test_compress
./build/test/ut_test_effective_sink
```

7 个单元测试覆盖：压缩、加密、mmap、space 单位、异步上下文、EffectiveSink 核心流程。
