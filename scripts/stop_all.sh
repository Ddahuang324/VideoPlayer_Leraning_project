#!/bin/bash

# YiboServer Complete Shutdown Script
# This script stops both the main application and the logger service

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LOGGER_SCRIPT="$SCRIPT_DIR/logger_service.sh"
PID_DIR="$PROJECT_ROOT/logs"
MAIN_PID_FILE="$PID_DIR/yiboserver.pid"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_banner() {
    echo -e "${BLUE}"
    echo "╔═══════════════════════════════════════════════════════╗"
    echo "║                                                       ║"
    echo "║          🛑 YiboServer Complete Shutdown              ║"
    echo "║                                                       ║"
    echo "╚═══════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

# Stop main application
stop_main_app() {
    print_info "Stopping main application..."
    
    if [ ! -f "$MAIN_PID_FILE" ]; then
        print_warning "Main application is not running (no PID file)"
        return 0
    fi
    
    PID=$(cat "$MAIN_PID_FILE")
    
    if ! ps -p "$PID" > /dev/null 2>&1; then
        print_warning "Main application is not running (stale PID file)"
        rm -f "$MAIN_PID_FILE"
        return 0
    fi
    
    print_info "Sending SIGTERM to main application (PID: $PID)..."
    kill -TERM "$PID"
    
    # Wait for graceful shutdown (max 10 seconds)
    for i in {1..10}; do
        if ! ps -p "$PID" > /dev/null 2>&1; then
            print_info "✅ Main application stopped successfully"
            rm -f "$MAIN_PID_FILE"
            return 0
        fi
        sleep 1
    done
    
    # Force kill if still running
    print_warning "Main application did not stop gracefully, forcing..."
    kill -KILL "$PID" 2>/dev/null
    rm -f "$MAIN_PID_FILE"
    print_info "Main application stopped (forced)"
}

# Stop logger service
stop_logger() {
    print_info "Stopping logger service..."
    
    "$LOGGER_SCRIPT" stop
    
    if [ $? -eq 0 ]; then
        print_info "✅ Logger service stopped"
        return 0
    else
        print_error "Failed to stop logger service"
        return 1
    fi
}

# Main shutdown sequence
main() {
    print_banner
    
    print_info "Shutting down all services..."
    echo ""
    
    # Step 1: Stop main application first
    stop_main_app
    echo ""
    
    # Wait a moment for logs to be flushed
    sleep 1
    
    # Step 2: Stop logger service
    stop_logger
    echo ""
    
    print_info "✅ All services stopped"
    echo ""
}

main

exit 0
