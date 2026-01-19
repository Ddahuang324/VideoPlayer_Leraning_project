# 易播服务端 C++17 重构项目

## 项目简介

易播服务端的现代化C++17重构版本，采用模块化设计，手写核心组件以深入理解底层原理。

## 构建要求

- **编译器**: GCC 7+ 或 Clang 5+ (支持C++17)
- **CMake**: 3.12+
- **依赖库**:
  - MySQL C API
  - SQLite3
  - OpenSSL
  - nlohmann/json

## 快速开始

### 1. 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get install libmysqlclient-dev libsqlite3-dev libssl-dev

# CentOS/RHEL
sudo yum install mysql-devel sqlite-devel openssl-devel
```

### 2. 编译项目

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 3. 运行程序

```bash
./yiboserver
```

## 项目结构

```
YiboServer/
├── include/          # 公共头文件
│   ├── common/      # 公共基础类
│   ├── network/     # 网络层
│   ├── concurrent/  # 并发层
│   ├── database/    # 数据层
│   └── utils/       # 工具类
├── src/             # 源文件实现
├── tests/           # 单元测试
├── third_party/     # 第三方库
├── docs/            # 文档
└── CMakeLists.txt   # CMake配置
```

## 开发规范

- 代码格式: 使用 `clang-format` (配置见 `.clang-format`)
- 命名规范: 类名使用CamelCase，成员变量使用m_前缀

## 许可证

MIT License
