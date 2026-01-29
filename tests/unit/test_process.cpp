#include <gtest/gtest.h>
#include "concurrent/Process.h"
#include <unistd.h>
#include <sys/wait.h>

using namespace yibo;

// 子进程入口函数：简单返回0
int SimpleChildFunc(CProcess* proc) {
    return 0;
}

// 子进程入口函数：返回特定退出码
int ExitCodeChildFunc(CProcess* proc) {
    return 42;
}

// 子进程入口函数：接收fd并发送回去
int EchoFDChildFunc(CProcess* proc) {
    int fd = -1;
    auto result = proc->RecvFD(fd);
    if (!result.IsOk()) {
        return 1;
    }

    result = proc->SendFD(fd);
    if (!result.IsOk()) {
        return 2;
    }

    close(fd);
    return 0;
}

TEST(ProcessTest, Constructor) {
    CProcess proc;
    EXPECT_EQ(proc.GetPid(), 0);
    EXPECT_EQ(proc.GetFd(), -1);
}

TEST(ProcessTest, MoveConstructor) {
    CProcess proc1;
    proc1.SetEntryFunction(SimpleChildFunc);

    CProcess proc2(std::move(proc1));
    EXPECT_EQ(proc1.GetFd(), -1);
}

TEST(ProcessTest, MoveAssignment) {
    CProcess proc1;
    proc1.SetEntryFunction(SimpleChildFunc);

    CProcess proc2;
    proc2 = std::move(proc1);
    EXPECT_EQ(proc1.GetFd(), -1);
}

TEST(ProcessTest, CreateSubProcessWithoutFunc) {
    CProcess proc;
    auto result = proc.CreateSubProcess();
    EXPECT_TRUE(result.IsErr());
    EXPECT_EQ(result.Error().code, ErrorCode::InvalidArgument);
}

TEST(ProcessTest, CreateSubProcessSuccess) {
    CProcess proc;
    proc.SetEntryFunction(SimpleChildFunc);

    auto result = proc.CreateSubProcess();
    ASSERT_TRUE(result.IsOk());

    EXPECT_GT(proc.GetPid(), 0);
    EXPECT_GE(proc.GetFd(), 0);

    // 等待子进程退出
    int status;
    waitpid(proc.GetPid(), &status, 0);
    EXPECT_TRUE(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 0);
}

TEST(ProcessTest, ChildProcessExitCode) {
    CProcess proc;
    proc.SetEntryFunction(ExitCodeChildFunc);

    auto result = proc.CreateSubProcess();
    ASSERT_TRUE(result.IsOk());

    // 等待子进程退出
    int status;
    waitpid(proc.GetPid(), &status, 0);
    EXPECT_TRUE(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 42);
}

TEST(ProcessTest, SendAndRecvFD) {
    CProcess proc;
    proc.SetEntryFunction(EchoFDChildFunc);

    auto result = proc.CreateSubProcess();
    ASSERT_TRUE(result.IsOk());

    // 创建一个临时文件描述符
    int pipe_fds[2];
    ASSERT_EQ(pipe(pipe_fds), 0);

    // 发送fd到子进程
    result = proc.SendFD(pipe_fds[0]);
    ASSERT_TRUE(result.IsOk());

    // 从子进程接收fd
    int recv_fd = -1;
    result = proc.RecvFD(recv_fd);
    ASSERT_TRUE(result.IsOk());
    EXPECT_GE(recv_fd, 0);

    // 清理
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    close(recv_fd);

    // 等待子进程退出
    int status;
    waitpid(proc.GetPid(), &status, 0);
    EXPECT_TRUE(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 0);
}

// 注意：SwitchDaemon测试会导致进程变为守护进程，不适合在单元测试中运行
// 这里只测试基本功能，实际守护进程功能需要在集成测试中验证
