#include <gtest/gtest.h>
#include "network/Epoll.h"
#include "network/EpollData.h"
#include <sys/socket.h>
#include <unistd.h>

using namespace yibo;

class EpollTest : public ::testing::Test {
protected:
    void SetUp() override {
        socketpair(AF_UNIX, SOCK_STREAM, 0, m_sockpair);
    }

    void TearDown() override {
        if (m_sockpair[0] != -1) close(m_sockpair[0]);
        if (m_sockpair[1] != -1) close(m_sockpair[1]);
    }

    int m_sockpair[2] = {-1, -1};
};

TEST_F(EpollTest, CreateSuccess) {
    CEpoll epoll;
    auto result = epoll.Create(1024);
    ASSERT_TRUE(result.IsOk());
}

TEST_F(EpollTest, AddSuccess) {
    CEpoll epoll;
    epoll.Create(1024);

    auto result = epoll.Add(m_sockpair[0], EpollData(m_sockpair[0]), EPOLLIN);
    ASSERT_TRUE(result.IsOk());
}

TEST_F(EpollTest, DelSuccess) {
    CEpoll epoll;
    epoll.Create(1024);
    epoll.Add(m_sockpair[0], EpollData(m_sockpair[0]), EPOLLIN);

    auto result = epoll.Del(m_sockpair[0]);
    ASSERT_TRUE(result.IsOk());
}

TEST_F(EpollTest, ModifySuccess) {
    CEpoll epoll;
    epoll.Create(1024);
    epoll.Add(m_sockpair[0], EpollData(m_sockpair[0]), EPOLLIN);

    auto result = epoll.Modify(m_sockpair[0], EPOLLOUT, EpollData(m_sockpair[0]));
    ASSERT_TRUE(result.IsOk());
}

TEST_F(EpollTest, WaitEventsTimeout) {
    CEpoll epoll;
    epoll.Create(1024);
    epoll.Add(m_sockpair[0], EpollData(m_sockpair[0]), EPOLLIN);

    EPEvents events;
    auto result = epoll.WaitEvents(events, 10);
    ASSERT_TRUE(result.IsOk());
    EXPECT_EQ(result.Value(), 0);
}

TEST_F(EpollTest, WaitEventsTriggered) {
    CEpoll epoll;
    epoll.Create(1024);
    epoll.Add(m_sockpair[0], EpollData(m_sockpair[0]), EPOLLIN);

    write(m_sockpair[1], "test", 4);

    EPEvents events;
    auto result = epoll.WaitEvents(events, 100);
    ASSERT_TRUE(result.IsOk());
    EXPECT_EQ(result.Value(), 1);
    EXPECT_EQ(events[0].data.fd, m_sockpair[0]);
    EXPECT_TRUE(events[0].events & EPOLLIN);
}

TEST_F(EpollTest, EpollDataFd) {
    EpollData data(42);
    EXPECT_EQ(static_cast<int>(data), 42);
}

TEST_F(EpollTest, EpollDataPtr) {
    int value = 100;
    EpollData data(&value);
    EXPECT_EQ(static_cast<void*>(data), &value);
}

TEST_F(EpollTest, MoveConstructor) {
    CEpoll epoll1;
    epoll1.Create(1024);

    CEpoll epoll2(std::move(epoll1));
    EXPECT_NE(static_cast<int>(epoll2), -1);
    EXPECT_EQ(static_cast<int>(epoll1), -1);
}
