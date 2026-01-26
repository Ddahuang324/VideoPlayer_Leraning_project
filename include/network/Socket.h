#pragma once

#include "common/Result.h"
#include "common/Error.h"
#include "common/Public.h"
#include <variant>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace yibo {

// Socket属性位掩码
enum SockAttr : unsigned {
    SOCK_ISSERVER   = 1 << 0,
    SOCK_ISNONBLOCK = 1 << 1,
    SOCK_ISUDP      = 1 << 2,
    SOCK_ISIP       = 1 << 3,
    SOCK_ISREUSE    = 1 << 4
};

struct CSockParam {
    using Address = std::variant<std::monostate, sockaddr_in, sockaddr_un>;

    Address address;
    unsigned attr = 0;

    static CSockParam MakeIPv4(const std::string& ip, uint16_t port, unsigned attr);
    static CSockParam MakeUnix(const std::string& path, unsigned attr);

    bool IsServer() const { return attr & SOCK_ISSERVER; }
    bool IsNonBlock() const { return attr & SOCK_ISNONBLOCK; }
    bool IsUDP() const { return attr & SOCK_ISUDP; }
    bool IsIP() const { return attr & SOCK_ISIP; }
    bool IsReuse() const { return attr & SOCK_ISREUSE; }

    const sockaddr* GetAddr() const;
    socklen_t GetAddrLen() const;
};

class CSocketBase {
public:
    virtual ~CSocketBase() = default;

    virtual Result<void, Error> Init(const CSockParam& param) = 0;
    virtual Result<ssize_t, Error> Send(std::string_view data) = 0;
    virtual Result<ssize_t, Error> Recv(Buffer& buffer, size_t max_size) = 0;
    virtual void Close() = 0;
    virtual operator int() const = 0;
};

class CSocket final : public CSocketBase {
public:
    CSocket() = default;
    ~CSocket() override { Close(); }

    // Disable copy semantics
    CSocket(const CSocket&) = delete;
    CSocket& operator=(const CSocket&) = delete;

    // Enable move semantics
    CSocket(CSocket&& other) noexcept
        : m_socket(std::exchange(other.m_socket, -1))
        , m_param(std::move(other.m_param)) {}

        
    CSocket& operator=(CSocket&& other) noexcept {
        if (this != &other) {
            Close();
            m_socket = std::exchange(other.m_socket, -1);
            m_param = std::move(other.m_param);
        }
        return *this;
    }

    Result<void, Error> Init(const CSockParam& param) override;
    Result<void, Error> InitFromExisting(int fd);
    Result<void, Error> Link();
    Result<UniquePtr<CSocket>, Error> Accept();
    Result<ssize_t, Error> Send(std::string_view data) override;
    Result<ssize_t, Error> Recv(Buffer& buffer, size_t max_size) override;
    void Close() override;
    operator int() const override { return m_socket; }

private:
    int m_socket = -1;
    CSockParam m_param;

    Result<void, Error> Bind();
    Result<void, Error> Listen(int backlog = 128);
    Result<void, Error> Connect();
};

} // namespace yibo
