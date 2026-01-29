#pragma once

#include "common/Public.h"
#include "common/Result.h"
#include "common/Error.h"
#include <functional>
#include <mutex>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace yibo {

/**
 * @brief 进程管理类，封装多进程创建和IPC通信
 *
 * 使用RAII管理进程资源和文件描述符
 * 支持Unix域套接字通信和文件描述符传递
 */
class CProcess {
public:
    CProcess();
    ~CProcess();

    // 禁止拷贝
    CProcess(const CProcess&) = delete;
    CProcess& operator=(const CProcess&) = delete;

    // 支持移动
    CProcess(CProcess&& other) noexcept;
    CProcess& operator=(CProcess&& other) noexcept;

    /**
     * @brief 设置子进程入口函数
     * @param func 子进程执行的函数，参数为CProcess指针，返回退出码
     */
    void SetEntryFunction(std::function<int(CProcess*)> func);

    /**
     * @brief 创建子进程
     * @return Result<int, Error> 成功返回0，失败返回错误信息
     */
    Result<int, Error> CreateSubProcess();

    /**
     * @brief 发送文件描述符到对端进程
     * @param fd 要发送的文件描述符
     * @return Result<int, Error> 成功返回0，失败返回错误信息
     */
    Result<int, Error> SendFD(int fd);

    /**
     * @brief 接收对端进程发送的文件描述符
     * @param fd 接收到的文件描述符（输出参数）
     * @return Result<int, Error> 成功返回0，失败返回错误信息
     */
    Result<int, Error> RecvFD(int& fd);

    /**
     * @brief 切换为守护进程
     * @return Result<int, Error> 成功返回0，失败返回错误信息
     */
    static Result<int, Error> SwitchDaemon();

    /**
     * @brief 获取进程ID
     */
    pid_t GetPid() const { return m_pid; }

    /**
     * @brief 获取通信文件描述符
     */
    int GetFd() const { return m_fd; }

private:
    /**
     * @brief 关闭继承的文件描述符
     */
    void CloseInheritedFds();

    /**
     * @brief 发送文件描述符的内部实现（需要加锁）
     */
    Result<int, Error> SendFDImpl(int fd);

    /**
     * @brief 接收文件描述符的内部实现（需要加锁）
     */
    Result<int, Error> RecvFDImpl(int& fd);

private:
    pid_t m_pid;                                // 进程ID（父进程中为子进程ID，子进程中为0）
    int m_fd;                                   // Unix域套接字文件描述符
    std::function<int(CProcess*)> m_func;       // 子进程入口函数
    std::mutex m_send_mutex;                    // 发送互斥锁
    std::mutex m_recv_mutex;                    // 接收互斥锁
};

} // namespace yibo
