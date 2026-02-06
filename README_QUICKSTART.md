# YiboServer 快速启动指南

## 🚀 一键启动/停止

### 启动所有服务

```bash
# 使用默认配置（config/server.json）
./scripts/start_all.sh

# 使用自定义配置
./scripts/start_all.sh config/server.dev.json
```

这个脚本会自动：
1. ✅ 检查所有必需的二进制文件
2. ✅ 启动日志服务器（独立进程）
3. ✅ 启动主应用程序
4. ✅ 显示服务状态和日志

### 停止所有服务

```bash
./scripts/stop_all.sh
```

这个脚本会自动：
1. ✅ 优雅地停止主应用程序
2. ✅ 停止日志服务器
3. ✅ 清理 PID 文件

### 查看状态

```bash
./scripts/start_all.sh status
```

## 📋 服务管理命令

### 日志服务器（独立管理）

```bash
# 启动日志服务器
./scripts/logger_service.sh start

# 停止日志服务器
./scripts/logger_service.sh stop

# 重启日志服务器
./scripts/logger_service.sh restart

# 查看状态
./scripts/logger_service.sh status
```

### 主应用程序（手动管理）

```bash
# 前台运行（用于调试）
./build/yiboserver -c config/server.json

# 后台运行
nohup ./build/yiboserver -c config/server.json > logs/yiboserver.log 2>&1 &
echo $! > logs/yiboserver.pid

# 停止
kill $(cat logs/yiboserver.pid)
```

## 📁 日志文件位置

```
logs/
├── logger.pid              # 日志服务器 PID
├── yiboserver.pid          # 主应用 PID
├── logger_service.log      # 日志服务器启动日志
├── yiboserver.log          # 主应用输出日志
└── server_YYYYMMDD_HHMMSS.log  # 应用业务日志（由日志服务器写入）
```

## 🔍 查看日志

```bash
# 查看主应用输出
tail -f logs/yiboserver.log

# 查看业务日志（由日志服务器写入）
tail -f logs/server_*.log

# 查看日志服务器日志
tail -f logs/logger_service.log

# 查看所有日志
tail -f logs/*.log
```

## ⚡ 快速示例

### 完整启动流程

```bash
# 1. 构建项目（首次或代码更新后）
cd /root/projects/YiboServer
mkdir -p build && cd build
cmake ..
make

# 2. 返回项目根目录
cd ..

# 3. 一键启动所有服务
./scripts/start_all.sh

# 输出示例：
# ╔═══════════════════════════════════════════════════════╗
# ║          🚀 YiboServer Complete Startup               ║
# ╚═══════════════════════════════════════════════════════╝
# 
# [INFO] Starting logger service...
# [INFO] ✅ Logger service started
# [INFO] Starting main application...
# [INFO] ✅ Main application started (PID: 12345)
# 
# ✅ All services started successfully!
```

### 查看运行状态

```bash
./scripts/start_all.sh status

# 输出示例：
# ═══════════════════════════════════════════════════════
#                     Service Status                      
# ═══════════════════════════════════════════════════════
# 
# Logger Service:
# [INFO] Logger service is running (PID: 12344)
# [INFO] Listening on UDP port: 9000
# 
# Main Application:
# [INFO] Running (PID: 12345)
# [INFO] Listening on TCP ports: 8080
```

### 停止所有服务

```bash
./scripts/stop_all.sh

# 输出示例：
# ╔═══════════════════════════════════════════════════════╗
# ║          🛑 YiboServer Complete Shutdown              ║
# ╚═══════════════════════════════════════════════════════╝
# 
# [INFO] Stopping main application...
# [INFO] ✅ Main application stopped successfully
# [INFO] Stopping logger service...
# [INFO] ✅ Logger service stopped
# 
# ✅ All services stopped
```

## 🛠️ 故障排查

### 问题：启动失败

```bash
# 检查二进制文件是否存在
ls -lh build/yiboserver
ls -lh build/yiboserver_logger

# 如果不存在，重新构建
cd build && make
```

### 问题：端口被占用

```bash
# 检查端口占用
netstat -tlnp | grep 8080  # 主应用端口
netstat -ulnp | grep 9000  # 日志服务器端口

# 停止占用端口的进程
kill <PID>
```

### 问题：日志未写入

```bash
# 1. 确认日志服务器运行
./scripts/logger_service.sh status

# 2. 检查日志目录权限
ls -ld logs/

# 3. 查看日志服务器日志
cat logs/logger_service.log

# 4. 查看主应用日志
cat logs/yiboserver.log
```

## 📊 监控脚本

创建一个简单的监控脚本 `scripts/monitor.sh`：

```bash
#!/bin/bash

watch -n 2 '
echo "=== YiboServer Status ==="
echo ""
echo "Logger Service:"
./scripts/logger_service.sh status 2>&1 | grep -E "running|PID|port"
echo ""
echo "Main Application:"
if [ -f logs/yiboserver.pid ]; then
    PID=$(cat logs/yiboserver.pid)
    if ps -p $PID > /dev/null 2>&1; then
        echo "Running (PID: $PID)"
        ps -p $PID -o %cpu,%mem,vsz,rss,cmd | tail -1
    else
        echo "Not running"
    fi
else
    echo "Not running"
fi
echo ""
echo "Recent logs (last 5 lines):"
tail -5 logs/server_*.log 2>/dev/null || echo "No logs"
'
```

## 🎯 开发工作流

### 开发模式（前台运行，便于调试）

```bash
# 终端 1: 启动日志服务器
./scripts/logger_service.sh start

# 终端 2: 前台运行主应用（可以看到实时输出）
./build/yiboserver -c config/server.json

# 停止: Ctrl+C 停止主应用，然后：
./scripts/logger_service.sh stop
```

### 生产模式（后台运行）

```bash
# 一键启动
./scripts/start_all.sh

# 查看日志
tail -f logs/server_*.log

# 一键停止
./scripts/stop_all.sh
```

## 📚 相关文档

- **详细日志使用指南**: `docs/LOGGER_USAGE_GUIDE.md`
- **日志服务器快速开始**: `docs/LOGGER_SERVER_QUICKSTART.md`
- **配置说明**: `docs/CONFIGURATION.md`

---

## 🎉 总结

现在你可以用一条命令启动整个系统：

```bash
./scripts/start_all.sh
```

系统会自动：
- ✅ 启动独立的日志进程
- ✅ 启动主应用程序
- ✅ 显示运行状态
- ✅ 提供日志查看命令

停止也很简单：

```bash
./scripts/stop_all.sh
```

享受你的高性能服务器吧！🚀
