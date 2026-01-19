#include "utils/StringUtils.h"
#include <sstream>
#include <iomanip>

namespace StringUtils {

std::vector<std::string_view> Split(std::string_view str, char delimiter) {
    std::vector<std::string_view> result;
    size_t start = 0;

    while (start < str.size()) {
        size_t end = str.find(delimiter, start);
        if (end == std::string_view::npos) {
            result.push_back(str.substr(start));
            break;
        }
        result.push_back(str.substr(start, end - start));
        start = end + 1;
    }

    return result;
}

std::vector<std::string_view> Split(std::string_view str, std::string_view delimiter) {
    std::vector<std::string_view> result;
    size_t start = 0;

    while (start < str.size()) {
        size_t end = str.find(delimiter, start);
        if (end == std::string_view::npos) {
            result.push_back(str.substr(start));
            break;
        }
        result.push_back(str.substr(start, end - start));
        start = end + delimiter.size();
    }

    return result;
}

std::string ToUpper(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string ToLower(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string UrlEncode(std::string_view str) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase;

    for (unsigned char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            oss << c;
        } else {
            oss << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
    }

    return oss.str();
}

std::string UrlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int value;
            std::istringstream iss(std::string(str.substr(i + 1, 2)));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }

    return result;
}

std::string Replace(std::string_view str, std::string_view from, std::string_view to) {
    if (from.empty()) return std::string(str);

    std::string result;
    size_t start = 0;

    while (start < str.size()) {
        size_t pos = str.find(from, start);
        if (pos == std::string_view::npos) {
            result.append(str.substr(start));
            break;
        }
        result.append(str.substr(start, pos - start));
        result.append(to);
        start = pos + from.size();
    }

    return result;
}

} // namespace StringUtils
