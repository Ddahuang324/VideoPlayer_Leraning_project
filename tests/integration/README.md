# YiboServer 集成测试

本目录包含 YiboServer 的集成测试套件，用于测试多个模块协同工作的场景。

## 测试文件概览

### 1. test_http_server_integration.cpp
**HTTP服务器全链路集成测试**

测试场景：
- ✅ 完整的HTTP请求处理流程（有效签名验证）
- ✅ 无效签名的请求处理
- ✅ 用户不存在的错误处理
- ✅ 多个并发请求处理（10个并发线程）
- ✅ 各种HTTP格式（GET/POST）

涵盖模块：
- 网络层 (Socket)
- HTTP解析器 (CHttpParser)
- 业务逻辑层 (CEdoyunPlayerServer)
- 数据库层 (CSqlite3Client)
- 加密工具 (Crypto::MD5)

### 2. test_database_integration.cpp
**数据库并发集成测试**

测试场景：
- ✅ 基本的CRUD操作（创建、读取、更新、删除）
- ✅ 事务处理（提交）
- ✅ 事务回滚
- ✅ 多线程并发读写（10线程 × 50操作）
- ✅ 使用线程池执行数据库查询
- ✅ SQL语法错误处理
- ✅ 大批量数据插入与查询（1000条记录）

涵盖模块：
- 数据库抽象层 (CDatabaseClient)
- SQLite3实现 (CSqlite3Client)
- 线程池 (CThreadPool)
- Result错误处理

### 3. test_end_to_end.cpp
**端到端集成测试**

测试场景：
- ✅ Socket创建和基本设置
- ✅ Epoll事件处理机制
- ✅ 完整的客户端请求-服务器响应流程
- ✅ 多个客户端并发连接（5个并发客户端）
- ✅ 连接中断错误处理

涵盖模块：
- 网络层完整栈 (Socket + Epoll)
- 服务器核心 (CServer)
- 业务逻辑 (CEdoyunPlayerServer)
- 数据库 (CSqlite3Client)
- 使用真实的Unix domain socket进行测试

### 4. test_logger_integration.cpp
**日志系统集成测试**

测试场景：
- ✅ 日志写入器基本功能
- ✅ 批量日志写入（1000条）
- ✅ 多线程并发写入（5线程 × 100条）
- ✅ 日志级别过滤（DEBUG/INFO/WARNING/ERROR/FATAL）
- ✅ 日志文件轮转
- ✅ 无效路径错误处理
- ✅ 日志格式验证
- ✅ 性能压力测试（10000条日志）
- ✅ 多进程日志写入模拟

涵盖模块：
- 日志系统 (CLogger, CLogWriter)
- 文件I/O
- 多线程并发写入

## 构建和运行

### 构建集成测试

```bash
cd /root/projects/YiboServer
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### 运行所有集成测试

```bash
./integration_tests
```

### 运行特定的集成测试

```bash
# 只运行HTTP服务器集成测试
./integration_tests --gtest_filter=HttpServerIntegrationTest.*

# 只运行数据库集成测试
./integration_tests --gtest_filter=DatabaseIntegrationTest.*

# 只运行端到端测试
./integration_tests --gtest_filter=EndToEndTest.*

# 只运行日志系统测试
./integration_tests --gtest_filter=LoggerIntegrationTest.*

# 运行特定的单个测试用例
./integration_tests --gtest_filter=HttpServerIntegrationTest.ValidHttpRequestFullFlow
```

### 查看详细输出

```bash
./integration_tests --gtest_color=yes --gtest_print_time=1
```

### 生成测试报告

```bash
./integration_tests --gtest_output=xml:test_report.xml
```

## 测试覆盖率

集成测试覆盖了以下关键路径：

1. **网络通信路径**: Socket → Epoll → CServer → CBusiness → 响应
2. **数据处理路径**: HTTP请求 → 解析 → 验证签名 → 数据库查询 → JSON响应
3. **并发处理路径**: 多线程/多进程 → 线程池 → 数据库 → 日志系统
4. **错误处理路径**: 网络错误 → 数据库错误 → 业务逻辑错误 → Result传播

## 性能基准

在标准开发机器上的预期性能：

- HTTP请求处理: < 10ms/请求
- 数据库查询: < 5ms/查询
- 日志写入: 10000条 < 10秒
- 并发处理: 10+并发客户端无阻塞

## 故障排查

### 测试失败常见原因

1. **数据库连接失败**
   - 检查SQLite3是否正确安装
   - 确认临时文件目录权限

2. **Socket测试失败**
   - 检查系统文件描述符限制 (`ulimit -n`)
   - 确认没有端口冲突

3. **日志测试失败**
   - 检查 /tmp 目录写入权限
   - 确认磁盘空间充足

### Debug模式运行

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
gdb --args ./integration_tests --gtest_filter=FailingTest.*
```

## 持续集成

集成测试应该在以下时机运行：

- ✅ 每次提交前
- ✅ Pull Request合并前
- ✅ 发布版本前
- ✅ 定期的夜间构建

## 贡献指南

添加新的集成测试时：

1. 在相应的测试文件中添加 `TEST_F` 用例
2. 确保测试名称清晰描述测试场景
3. 使用 `EXPECT_*` 和 `ASSERT_*` 进行断言
4. 在 SetUp() 中初始化测试环境
5. 在 TearDown() 中清理资源
6. 添加注释说明测试目的

## 参考文档

- [GoogleTest文档](https://google.github.io/googletest/)
- [项目架构说明](../../CLAUDE.md)
- [单元测试](../unit/)
