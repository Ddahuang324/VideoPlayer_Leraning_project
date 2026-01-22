#include "concurrent/Process.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>
#include <utility>

namespace yibo {

CProcess::CProcess()
    : m_pid(0)
    , m_fd(-1)
    , m_func(nullptr) {
}

CProcess::~CProcess() {
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
}

CProcess::CProcess(CProcess&& other) noexcept
    : m_pid(std::exchange(other.m_pid, 0))
    , m_fd(std::exchange(other.m_fd, -1))
    , m_func(std::move(other.m_func)) {
}

CProcess& CProcess::operator=(CProcess&& other) noexcept {
    if (this != &other) {
        if (m_fd >= 0) {
            close(m_fd);
        }
        m_pid = std::exchange(other.m_pid, 0);
        m_fd = std::exchange(other.m_fd, -1);
        m_func = std::move(other.m_func);
    }
    return *this;
}

void CProcess::SetEntryFunction(std::function<int(CProcess*)> func) {
    m_func = func;
}

Result<int, Error> CProcess::CreateSubProcess() {
    if (!m_func) {
        return Result<int, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Entry function not set")
        );
    }

    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        return Result<int, Error>::Err(
            Error(ErrorCode::SocketError, "socketpair failed: " + std::string(strerror(errno)))
        );
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        return Result<int, Error>::Err(
            Error(ErrorCode::Unknown, "fork failed: " + std::string(strerror(errno)))
        );
    }

    if (pid == 0) {
        // 子进程
        close(fds[0]);
        m_fd = fds[1];
        m_pid = 0;
        CloseInheritedFds();
        int ret = m_func(this);
        exit(ret);
    } else {
        // 父进程
        close(fds[1]);
        m_fd = fds[0];
        m_pid = pid;
    }

    return Result<int, Error>::Ok(0);
}

Result<int, Error> CProcess::SendFD(int fd) {
    std::lock_guard<std::mutex> lock(m_send_mutex);
    return SendFDImpl(fd);
}

Result<int, Error> CProcess::SendFDImpl(int fd) {
    struct msghdr msg;
    struct iovec iov[1];
    char buf[1] = {0};

    iov[0].iov_base = buf;
    iov[0].iov_len = 1;

    union {
        char control[CMSG_SPACE(sizeof(int))];
        struct cmsghdr align;
    } u;

    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = u.control;
    msg.msg_controllen = sizeof(u.control);
    msg.msg_flags = 0;

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    *reinterpret_cast<int*>(CMSG_DATA(cmsg)) = fd;

    if (sendmsg(m_fd, &msg, 0) < 0) {
        return Result<int, Error>::Err(
            Error(ErrorCode::NetworkError, "sendmsg failed: " + std::string(strerror(errno)))
        );
    }

    return Result<int, Error>::Ok(0);
}

Result<int, Error> CProcess::RecvFD(int& fd) {
    std::lock_guard<std::mutex> lock(m_recv_mutex);
    return RecvFDImpl(fd);
}

Result<int, Error> CProcess::RecvFDImpl(int& fd) {
    struct msghdr msg;
    struct iovec iov[1];
    char buf[1];

    iov[0].iov_base = buf;
    iov[0].iov_len = 1;

    union {
        char control[CMSG_SPACE(sizeof(int))];
        struct cmsghdr align;
    } u;

    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = u.control;
    msg.msg_controllen = sizeof(u.control);
    msg.msg_flags = 0;

    ssize_t n = recvmsg(m_fd, &msg, 0);
    if (n <= 0) {
        return Result<int, Error>::Err(
            Error(ErrorCode::NetworkError, "recvmsg failed: " + std::string(strerror(errno)))
        );
    }

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg == nullptr || cmsg->cmsg_len != CMSG_LEN(sizeof(int)) ||
        cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
        return Result<int, Error>::Err(
            Error(ErrorCode::NetworkError, "Invalid control message")
        );
    }

    fd = *reinterpret_cast<int*>(CMSG_DATA(cmsg));
    return Result<int, Error>::Ok(0);
}

Result<int, Error> CProcess::SwitchDaemon() {
    // 第一次fork
    pid_t pid = fork();
    if (pid < 0) {
        return Result<int, Error>::Err(
            Error(ErrorCode::Unknown, "First fork failed: " + std::string(strerror(errno)))
        );
    }
    if (pid > 0) {
        exit(0);  // 父进程退出
    }

    // 创建新会话
    if (setsid() < 0) {
        return Result<int, Error>::Err(
            Error(ErrorCode::Unknown, "setsid failed: " + std::string(strerror(errno)))
        );
    }

    // 第二次fork
    pid = fork();
    if (pid < 0) {
        return Result<int, Error>::Err(
            Error(ErrorCode::Unknown, "Second fork failed: " + std::string(strerror(errno)))
        );
    }
    if (pid > 0) {
        exit(0);  // 第一个子进程退出
    }

    // 改变工作目录
    if (chdir("/") < 0) {
        return Result<int, Error>::Err(
            Error(ErrorCode::FileError, "chdir failed: " + std::string(strerror(errno)))
        );
    }

    // 重设文件权限掩码
    umask(0);

    // 关闭标准输入输出错误
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // 重定向到/dev/null
    int fd = open("/dev/null", O_RDWR);
    if (fd >= 0) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }

    return Result<int, Error>::Ok(0);
}

void CProcess::CloseInheritedFds() {
    // 关闭除了m_fd之外的所有文件描述符
    // 简单实现：关闭0-1023范围内的fd（除了m_fd和标准流）
    for (int i = 3; i < 1024; i++) {
        if (i != m_fd) {
            close(i);
        }
    }
}

} // namespace yibo
