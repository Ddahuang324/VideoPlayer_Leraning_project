#pragma once

#include "ErrorCode.h"
#include <exception>
#include <string>

namespace yibo {

class ServerException : public std::exception {
public:
    explicit ServerException(std::string message)
        : m_message(std::move(message)) {}

    const char* what() const noexcept override {
        return m_message.c_str();
    }

    virtual ErrorCode Code() const noexcept = 0;

protected:
    std::string m_message;
};

class NetworkException : public ServerException {
public:
    using ServerException::ServerException;
    ErrorCode Code() const noexcept override {
        return ErrorCode::NetworkError;
    }
};

class DatabaseException : public ServerException {
public:
    using ServerException::ServerException;
    ErrorCode Code() const noexcept override {
        return ErrorCode::DatabaseError;
    }
};

class HttpException : public ServerException {
public:
    HttpException(std::string message, int status_code)
        : ServerException(std::move(message))
        , m_status_code(status_code) {}

    ErrorCode Code() const noexcept override {
        return ErrorCode::HttpError;
    }

    int StatusCode() const noexcept { return m_status_code; }

private:
    int m_status_code;
};

class LogicException : public ServerException {
public:
    using ServerException::ServerException;
    ErrorCode Code() const noexcept override {
        return ErrorCode::LogicError;
    }
};

} // namespace yibo
