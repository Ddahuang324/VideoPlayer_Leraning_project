#pragma once

#include <memory>
#include <string>

// 智能指针别名
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

// Buffer类型（继承std::string）
class Buffer : public std::string {
public:
    using std::string::string;
    operator const char*() const { return c_str(); }
};
