#pragma once

#include <string_view>

namespace yibo {

enum class ErrorCode {
    // 成功
    Success = 0,

    // 通用错误 (1-99)
    Unknown = 1,
    InvalidArgument = 2,
    NotImplemented = 3,
    OutOfMemory = 4,

    // 网络错误 (100-199)
    NetworkError = 100,
    ConnectionRefused = 101,
    ConnectionTimeout = 102,
    ConnectionClosed = 103,
    SocketError = 104,
    SocketCreateFailed = 105,
    BindFailed = 106,
    ListenFailed = 107,
    AcceptFailed = 108,
    ConnectFailed = 109,
    SendFailed = 110,
    RecvFailed = 111,
    WouldBlock = 112,
    Interrupted = 113,
    AlreadyInitialized = 114,
    NotInitialized = 115,
    SetNonBlockFailed = 116,
    SetSockOptFailed = 117,

    // HTTP错误 (200-299)
    HttpError = 200,
    HttpBadRequest = 201,
    HttpUnauthorized = 202,
    HttpNotFound = 203,
    HttpInternalError = 204,

    // 数据库错误 (300-399)
    DatabaseError = 300,
    DatabaseConnectionFailed = 301,
    DatabaseQueryFailed = 302,
    DatabaseTransactionFailed = 303,

    // 业务逻辑错误 (400-499)
    LogicError = 400,
    InvalidState = 401,
    AuthenticationFailed = 402,
    PermissionDenied = 403,

    // 文件I/O错误 (500-599)
    FileError = 500,
    FileNotFound = 501,
    FilePermissionDenied = 502,
    FileReadError = 503,
    FileWriteError = 504,
};

inline std::string_view ErrorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success: return "Success";
        case ErrorCode::Unknown: return "Unknown error";
        case ErrorCode::InvalidArgument: return "Invalid argument";
        case ErrorCode::NotImplemented: return "Not implemented";
        case ErrorCode::OutOfMemory: return "Out of memory";
        case ErrorCode::NetworkError: return "Network error";
        case ErrorCode::ConnectionRefused: return "Connection refused";
        case ErrorCode::ConnectionTimeout: return "Connection timeout";
        case ErrorCode::ConnectionClosed: return "Connection closed";
        case ErrorCode::SocketError: return "Socket error";
        case ErrorCode::SocketCreateFailed: return "Socket create failed";
        case ErrorCode::BindFailed: return "Bind failed";
        case ErrorCode::ListenFailed: return "Listen failed";
        case ErrorCode::AcceptFailed: return "Accept failed";
        case ErrorCode::ConnectFailed: return "Connect failed";
        case ErrorCode::SendFailed: return "Send failed";
        case ErrorCode::RecvFailed: return "Recv failed";
        case ErrorCode::WouldBlock: return "Would block";
        case ErrorCode::Interrupted: return "Interrupted";
        case ErrorCode::AlreadyInitialized: return "Already initialized";
        case ErrorCode::NotInitialized: return "Not initialized";
        case ErrorCode::SetNonBlockFailed: return "Set non-block failed";
        case ErrorCode::SetSockOptFailed: return "Set socket option failed";
        case ErrorCode::HttpError: return "HTTP error";
        case ErrorCode::HttpBadRequest: return "Bad request";
        case ErrorCode::HttpUnauthorized: return "Unauthorized";
        case ErrorCode::HttpNotFound: return "Not found";
        case ErrorCode::HttpInternalError: return "Internal server error";
        case ErrorCode::DatabaseError: return "Database error";
        case ErrorCode::DatabaseConnectionFailed: return "Database connection failed";
        case ErrorCode::DatabaseQueryFailed: return "Database query failed";
        case ErrorCode::DatabaseTransactionFailed: return "Database transaction failed";
        case ErrorCode::LogicError: return "Logic error";
        case ErrorCode::InvalidState: return "Invalid state";
        case ErrorCode::AuthenticationFailed: return "Authentication failed";
        case ErrorCode::PermissionDenied: return "Permission denied";
        case ErrorCode::FileError: return "File error";
        case ErrorCode::FileNotFound: return "File not found";
        case ErrorCode::FilePermissionDenied: return "File permission denied";
        case ErrorCode::FileReadError: return "File read error";
        case ErrorCode::FileWriteError: return "File write error";
        default: return "Unknown error code";
    }
}

} // namespace yibo
