#include "network/Socket.h"

namespace yibo {

CSockParam CSockParam::MakeIPv4(const std::string& ip, uint16_t port, unsigned attr) {
    CSockParam param;
    param.attr = attr | SOCK_ISIP;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        return CSockParam{};
    }

    param.address = addr;
    return param;
}

CSockParam CSockParam::MakeUnix(const std::string& path, unsigned attr) {
    CSockParam param;
    param.attr = attr;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    if (path.size() >= sizeof(addr.sun_path)) {
        return CSockParam{};
    }

    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    param.address = addr;
    return param;
}

const sockaddr* CSockParam::GetAddr() const {
    return std::visit([](auto&& addr) -> const sockaddr* {
        using T = std::decay_t<decltype(addr)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return nullptr;
        } else {
            return reinterpret_cast<const sockaddr*>(&addr);
        }
    }, address);
}

socklen_t CSockParam::GetAddrLen() const {
    return std::visit([](auto&& addr) -> socklen_t {
        using T = std::decay_t<decltype(addr)>;
        if constexpr (std::is_same_v<T, sockaddr_in>) {
            return sizeof(sockaddr_in);
        } else if constexpr (std::is_same_v<T, sockaddr_un>) {
            return sizeof(sockaddr_un);
        } else {
            return 0;
        }
    }, address);
}

Result<void, Error> CSocket::Init(const CSockParam& param) {
    if (m_socket != -1) {
        return Result<void, Error>::Err(
            Error(ErrorCode::AlreadyInitialized, "Socket already initialized")
        );
    }

    return std::visit([this, &param](auto&& addr) -> Result<void, Error> {
        using T = std::decay_t<decltype(addr)>;

        if constexpr (std::is_same_v<T, std::monostate>) {
            return Result<void, Error>::Err(
                Error(ErrorCode::InvalidArgument, "Address not initialized")
            );
        } else {
            int domain = std::is_same_v<T, sockaddr_in> ? AF_INET : AF_UNIX;
            int type = (param.attr & SOCK_ISUDP) ? SOCK_DGRAM : SOCK_STREAM;

            m_socket = socket(domain, type, 0);
            if (m_socket == -1) {
                return Result<void, Error>::Err(
                    Error(ErrorCode::SocketCreateFailed, strerror(errno))
                );
            }

            //是否设置为非阻塞模式
            if (param.attr & SOCK_ISNONBLOCK) {
                int flags = fcntl(m_socket, F_GETFL, 0);
                if (flags == -1 || fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
                    Close();
                    return Result<void, Error>::Err(
                        Error(ErrorCode::SetNonBlockFailed, strerror(errno))
                    );
                }
            }

            //是否实现地址重用
            if (param.attr & SOCK_ISREUSE) {
                int opt = 1;
                if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
                    Close();
                    return Result<void, Error>::Err(
                        Error(ErrorCode::SetSockOptFailed, strerror(errno))
                    );
                }
            }

            m_param = param;
            return Result<void, Error>::Ok();
        }
    }, param.address);
}

Result<void, Error> CSocket::Link() {
    if (m_socket == -1) {
        return Result<void, Error>::Err(
            Error(ErrorCode::NotInitialized, "Socket not initialized")
        );
    }

    if (m_param.IsServer()) {
        auto bind_result = Bind();
        if (bind_result.IsErr()) {
            return bind_result;
        }

        if (!m_param.IsUDP()) {
            return Listen();
        }

        return Result<void, Error>::Ok();
    } else {
        return Connect();
    }
}

Result<UniquePtr<CSocket>, Error> CSocket::Accept() {
    if (m_socket == -1) {
        return Result<UniquePtr<CSocket>, Error>::Err(
            Error(ErrorCode::NotInitialized, "Socket not initialized")
        );
    }

    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(m_socket, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);

    if (client_fd == -1) {

        //阻塞或者非阻塞模式下无连接请求
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return Result<UniquePtr<CSocket>, Error>::Err(
                Error(ErrorCode::WouldBlock, "No pending connections")
            );
        }
        return Result<UniquePtr<CSocket>, Error>::Err(
            Error(ErrorCode::AcceptFailed, strerror(errno))
        );
    }

    auto client = MakeUnique<CSocket>();
    client->m_socket = client_fd;
    client->m_param = m_param;
    //套接字为客户端，去掉服务器标志
    client->m_param.attr &= ~SOCK_ISSERVER;
    //&= ~ 的清零过程？
    // 例如：如果 m_param.attr 是 0x0001 (SOCK_ISSERVER = 1 << 0)
    // 那么 ~SOCK_ISSERVER 就是 0xFFFE (所有位都取反)
    // 然后与原值进行按位与操作，将服务器标志位清零

    return Result<UniquePtr<CSocket>, Error>::Ok(std::move(client));
}

Result<ssize_t, Error> CSocket::Send(std::string_view data) {
    if (m_socket == -1) {
        return Result<ssize_t, Error>::Err(
            Error(ErrorCode::NotInitialized, "Socket not initialized")
        );
    }

    ssize_t ret = send(m_socket, data.data(), data.size(), 0);

    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return Result<ssize_t, Error>::Err(
                Error(ErrorCode::WouldBlock, "Send buffer full")
            );
        }
        //EINTR 信号中断
        if (errno == EINTR) {
            return Result<ssize_t, Error>::Err(
                Error(ErrorCode::Interrupted, "Send interrupted by signal")
            );
        }
        return Result<ssize_t, Error>::Err(
            Error(ErrorCode::SendFailed, strerror(errno))
        );
    }

    return Result<ssize_t, Error>::Ok(ret);
}

Result<ssize_t, Error> CSocket::Recv(Buffer& buffer, size_t max_size) {
    if (m_socket == -1) {
        return Result<ssize_t, Error>::Err(
            Error(ErrorCode::NotInitialized, "Socket not initialized")
        );
    }

    buffer.resize(max_size);
    ssize_t ret = recv(m_socket, buffer.data(), max_size, 0);

    if (ret < 0) {
        //阻塞或者非阻塞模式下无数据可读
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return Result<ssize_t, Error>::Err(
                Error(ErrorCode::WouldBlock, "No data available")
            );
        }
        if (errno == EINTR) {
            return Result<ssize_t, Error>::Err(
                Error(ErrorCode::Interrupted, "Recv interrupted by signal")
            );
        }
        return Result<ssize_t, Error>::Err(
            Error(ErrorCode::RecvFailed, strerror(errno))
        );
    }

    if (ret == 0) {
        return Result<ssize_t, Error>::Err(
            Error(ErrorCode::ConnectionClosed, "Peer closed connection")
        );
    }

    buffer.resize(ret);
    return Result<ssize_t, Error>::Ok(ret);
}

void CSocket::Close() {
    if (m_socket != -1) {
        close(m_socket);
        m_socket = -1;
    }
}

Result<void, Error> CSocket::Bind() {
    const sockaddr* addr = m_param.GetAddr();
    socklen_t addr_len = m_param.GetAddrLen();

    if (bind(m_socket, addr, addr_len) == -1) {
        return Result<void, Error>::Err(
            Error(ErrorCode::BindFailed, strerror(errno))
        );
    }
    return Result<void, Error>::Ok();
}

Result<void, Error> CSocket::Listen(int backlog) {
    if (listen(m_socket, backlog) == -1) {
        return Result<void, Error>::Err(
            Error(ErrorCode::ListenFailed, strerror(errno))
        );
    }
    return Result<void, Error>::Ok();
}

Result<void, Error> CSocket::Connect() {
    const sockaddr* addr = m_param.GetAddr();
    socklen_t addr_len = m_param.GetAddrLen();

    if (connect(m_socket, addr, addr_len) == -1) {
        if (errno == EINPROGRESS) {
            return Result<void, Error>::Err(
                Error(ErrorCode::WouldBlock, "Connection in progress")
            );
        }
        return Result<void, Error>::Err(
            Error(ErrorCode::ConnectFailed, strerror(errno))
        );
    }
    return Result<void, Error>::Ok();
}

} // namespace yibo
