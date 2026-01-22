#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <mutex>
#include <condition_variable>

namespace yibo {

// Buffer类型：直接使用std::string
using Buffer = std::string;

// 如果需要视图语义，使用string_view
using BufferView = std::string_view;

// 智能指针别名
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// 工厂函数
template<typename T, typename... Args>
inline UniquePtr<T> MakeUnique(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
inline SharedPtr<T> MakeShared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// Synchronization primitives type aliases
using Mutex = std::mutex;
using RecursiveMutex = std::recursive_mutex;
using TimedMutex = std::timed_mutex;

template<typename Mutex>
using LockGuard = std::lock_guard<Mutex>;

template<typename Mutex>
using UniqueLock = std::unique_lock<Mutex>;

using ConditionVariable = std::condition_variable;
using ConditionVariableAny = std::condition_variable_any;

using OnceFlag = std::once_flag;

} // namespace yibo

