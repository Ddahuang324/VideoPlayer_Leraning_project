#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>

namespace StringUtils {

// Trim操作
inline std::string_view TrimLeft(std::string_view str) {
    auto pos = str.find_first_not_of(" \t\r\n");
    return pos == std::string_view::npos ? "" : str.substr(pos);
}

inline std::string_view TrimRight(std::string_view str) {
    auto pos = str.find_last_not_of(" \t\r\n");
    return pos == std::string_view::npos ? "" : str.substr(0, pos + 1);
}

inline std::string_view Trim(std::string_view str) {
    return TrimLeft(TrimRight(str));
}

// 字符串分割
std::vector<std::string_view> Split(std::string_view str, char delimiter);
std::vector<std::string_view> Split(std::string_view str, std::string_view delimiter);

// 大小写转换
std::string ToUpper(std::string_view str);
std::string ToLower(std::string_view str);

// 判断前缀/后缀
inline bool StartsWith(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() &&
           str.substr(0, prefix.size()) == prefix;
}

inline bool EndsWith(std::string_view str, std::string_view suffix) {
    return str.size() >= suffix.size() &&
           str.substr(str.size() - suffix.size()) == suffix;
}

// URL编码/解码
std::string UrlEncode(std::string_view str);
std::string UrlDecode(std::string_view str);

// 字符串替换
std::string Replace(std::string_view str, std::string_view from, std::string_view to);

// 字符串连接
template<typename Container>
std::string Join(const Container& items, std::string_view separator) {
    if (items.empty()) return "";

    std::string result;
    auto it = items.begin();
    result += *it++;

    for (; it != items.end(); ++it) {
        result += separator;
        result += *it;
    }

    return result;
}

} // namespace StringUtils
