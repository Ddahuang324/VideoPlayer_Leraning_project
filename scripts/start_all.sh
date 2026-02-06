#!/bin/bash

# YiboServer Complete Startup Script
# This script starts both the logger service and the main application

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LOGGER_SCRIPT="$SCRIPT_DIR/logger_service.sh"
MAIN_BIN="$PROJECT_ROOT/build/yiboserver"
CONFIG_FILE="${1:-$PROJECT_ROOT/config/server.json}"
PID_DIR="$PROJECT_ROOT/logs"
MAIN_PID_FILE="$PID_DIR/yiboserver.pid"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_banner() {
    echo -e "${BLUE}"
    echo "╔═══════════════════════════════════════════════════════╗"
    echo "║                                                       ║"
    echo "║          🚀 YiboServer Complete Startup               ║"
    echo "║                                                       ║"
    echo "╚═══════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Check if binaries exist
check_binaries() {
    if [ ! -f "$MAIN_BIN" ]; then
        print_error "Main application not found: $MAIN_BIN"
        print_info "Please build the project first:"
        echo "  cd $PROJECT_ROOT/build"
        echo "  cmake .."
        echo "  make"
        exit 1
    fi
    
    if [ ! -x "$LOGGER_SCRIPT" ]; then
        print_error "Logger service script not found or not executable: $LOGGER_SCRIPT"
        exit 1
    fi
}

# Start logger service
start_logger() {
    print_info "Starting logger service..."
    
    if "$LOGGER_SCRIPT" status > /dev/null 2>&1; then
        print_warning "Logger service is already running"
        return 0
    fi
    
    "$LOGGER_SCRIPT" start
    
    if [ $? -eq 0 ]; then
        print_info "✅ Logger service started"
        sleep 1  # Wait for logger to be ready
        return 0
    else
        print_error "Failed to start logger service"
        return 1
    fi
}

# Start main application
start_main_app() {
    print_info "Starting main application..."
    
    # Check if config file exists
    if [ ! -f "$CONFIG_FILE" ]; then
        print_warning "Config file not found: $CONFIG_FILE"
        print_info "Using default configuration"
        CONFIG_FILE="$PROJECT_ROOT/config/server.json"
    fi
    
    # Check if already running
    if [ -f "$MAIN_PID_FILE" ]; then
        PID=$(cat "$MAIN_PID_FILE")
        if ps -p "$PID" > /dev/null 2>&1; then
            print_warning "Main application is already running (PID: $PID)"
            return 0
        else
            print_warning "Stale PID file found, removing..."
            rm -f "$MAIN_PID_FILE"
        fi
    fi
    
    # Create PID directory
    mkdir -p "$PID_DIR"
    
    # Start main application in background
    cd "$PROJECT_ROOT"
    nohup "$MAIN_BIN" -c "$CONFIG_FILE" > "$PID_DIR/yiboserver.log" 2>&1 &
    MAIN_PID=$!
    
    # Save PID
    echo "$MAIN_PID" > "$MAIN_PID_FILE"
    
    # Wait and check if it's running
    sleep 1
    if ps -p "$MAIN_PID" > /dev/null 2>&1; then
        print_info "✅ Main application started (PID: $MAIN_PID)"
        print_info "Config: $CONFIG_FILE"
        print_info "Log: $PID_DIR/yiboserver.log"
        return 0
    else
        print_error "Failed to start main application"
        print_error "Check log: $PID_DIR/yiboserver.log"
        rm -f "$MAIN_PID_FILE"
        return 1
    fi
}

# Show status
show_status() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}                    Service Status                      ${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
    echo ""
    
    # Logger service status
    echo -e "${YELLOW}Logger Service:${NC}"
    "$LOGGER_SCRIPT" status
    echo ""
    
    # Main application status
    echo -e "${YELLOW}Main Application:${NC}"
    if [ -f "$MAIN_PID_FILE" ]; then
        PID=$(cat "$MAIN_PID_FILE")
        if ps -p "$PID" > /dev/null 2>&1; then
            print_info "Running (PID: $PID)"
            
            # Show listening ports
            PORTS=$(netstat -tlnp 2>/dev/null | grep "$PID" | grep -oP ':\K[0-9]+' | tr '\n' ' ')
            if [ -n "$PORTS" ]; then
                print_info "Listening on TCP ports: $PORTS"
            fi
        else
            print_warning "Not running (stale PID file)"
        fi
    else
        print_warning "Not running"
    fi
    
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
    echo ""
    
    # Show recent logs
    echo -e "${YELLOW}Recent Application Logs:${NC}"
    if [ -f "$PID_DIR/yiboserver.log" ]; then
        tail -10 "$PID_DIR/yiboserver.log"
    else
        echo "No logs available"
    fi
    
    echo ""
}

# Main startup sequence
main() {
    print_banner
    
    print_info "Project root: $PROJECT_ROOT"
    print_info "Configuration: $CONFIG_FILE"
    echo ""
    
    # Check binaries
    check_binaries
    
    # Start services
    print_info "Starting services..."
    echo ""
    
    # Step 1: Start logger service
    if ! start_logger; then
        print_error "Failed to start logger service, aborting..."
        exit 1
    fi
    
    echo ""
    
    # Step 2: Start main application
    if ! start_main_app; then
        print_error "Failed to start main application"
        print_warning "Logger service is still running, you may want to stop it"
        exit 1
    fi
    
    echo ""
    
    # Show status
    show_status
    
    # Show helpful commands
    echo -e "${GREEN}✅ All services started successfully!${NC}"
    echo ""
    echo -e "${YELLOW}Useful commands:${NC}"
    echo "  View logs:        tail -f $PID_DIR/yiboserver.log"
    echo "  View logger logs: tail -f $PID_DIR/server_*.log"
    echo "  Check status:     $0 status"
    echo "  Stop all:         $SCRIPT_DIR/stop_all.sh"
    echo ""
}

# Handle command line arguments
case "${1}" in
    status)
        show_status
        ;;
    *)
        main
        ;;
esac

exit 0
