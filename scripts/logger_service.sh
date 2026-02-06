#!/bin/bash

# YiboServer Logger Service Startup Script
# This script starts the logger service as a background daemon

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LOGGER_BIN="$PROJECT_ROOT/build/yiboserver_logger"
PID_FILE="$PROJECT_ROOT/logs/logger.pid"
LOG_DIR="$PROJECT_ROOT/logs"
CONFIG_FILE="$PROJECT_ROOT/config/server.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored messages
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Check if logger binary exists
check_binary() {
    if [ ! -f "$LOGGER_BIN" ]; then
        print_error "Logger binary not found: $LOGGER_BIN"
        print_info "Please build the project first: cd build && cmake .. && make"
        exit 1
    fi
}

# Start logger service
start_logger() {
    check_binary
    
    # Check if already running
    if [ -f "$PID_FILE" ]; then
        PID=$(cat "$PID_FILE")
        if ps -p "$PID" > /dev/null 2>&1; then
            print_warning "Logger service is already running (PID: $PID)"
            return 0
        else
            print_warning "Stale PID file found, removing..."
            rm -f "$PID_FILE"
        fi
    fi
    
    # Create log directory
    mkdir -p "$LOG_DIR"
    
    print_info "Starting logger service..."
    
    # Start logger in background
    nohup "$LOGGER_BIN" -c "$CONFIG_FILE" > "$LOG_DIR/logger_service.log" 2>&1 &
    LOGGER_PID=$!
    
    # Save PID
    echo "$LOGGER_PID" > "$PID_FILE"
    
    # Wait a moment and check if it's running
    sleep 1
    if ps -p "$LOGGER_PID" > /dev/null 2>&1; then
        print_info "Logger service started successfully (PID: $LOGGER_PID)"
        print_info "Log directory: $LOG_DIR"
        print_info "Service log: $LOG_DIR/logger_service.log"
        return 0
    else
        print_error "Failed to start logger service"
        rm -f "$PID_FILE"
        return 1
    fi
}

# Stop logger service
stop_logger() {
    if [ ! -f "$PID_FILE" ]; then
        print_warning "Logger service is not running (no PID file)"
        return 0
    fi
    
    PID=$(cat "$PID_FILE")
    
    if ! ps -p "$PID" > /dev/null 2>&1; then
        print_warning "Logger service is not running (stale PID file)"
        rm -f "$PID_FILE"
        return 0
    fi
    
    print_info "Stopping logger service (PID: $PID)..."
    kill -TERM "$PID"
    
    # Wait for graceful shutdown (max 10 seconds)
    for i in {1..10}; do
        if ! ps -p "$PID" > /dev/null 2>&1; then
            print_info "Logger service stopped successfully"
            rm -f "$PID_FILE"
            return 0
        fi
        sleep 1
    done
    
    # Force kill if still running
    print_warning "Logger service did not stop gracefully, forcing..."
    kill -KILL "$PID" 2>/dev/null
    rm -f "$PID_FILE"
    print_info "Logger service stopped (forced)"
}

# Check logger service status
status_logger() {
    if [ ! -f "$PID_FILE" ]; then
        print_info "Logger service is not running"
        return 1
    fi
    
    PID=$(cat "$PID_FILE")
    
    if ps -p "$PID" > /dev/null 2>&1; then
        print_info "Logger service is running (PID: $PID)"
        
        # Show port information
        PORT=$(netstat -ulnp 2>/dev/null | grep "$PID" | grep -oP ':\K[0-9]+' | head -1)
        if [ -n "$PORT" ]; then
            print_info "Listening on UDP port: $PORT"
        fi
        
        # Show log directory
        print_info "Log directory: $LOG_DIR"
        
        return 0
    else
        print_warning "Logger service is not running (stale PID file)"
        rm -f "$PID_FILE"
        return 1
    fi
}

# Restart logger service
restart_logger() {
    print_info "Restarting logger service..."
    stop_logger
    sleep 1
    start_logger
}

# Show usage
usage() {
    cat << EOF
Usage: $0 {start|stop|restart|status}

Commands:
    start       Start the logger service
    stop        Stop the logger service
    restart     Restart the logger service
    status      Check logger service status

Examples:
    $0 start        # Start logger service
    $0 status       # Check if running
    $0 stop         # Stop logger service

EOF
}

# Main script logic
case "$1" in
    start)
        start_logger
        ;;
    stop)
        stop_logger
        ;;
    restart)
        restart_logger
        ;;
    status)
        status_logger
        ;;
    *)
        usage
        exit 1
        ;;
esac

exit $?
