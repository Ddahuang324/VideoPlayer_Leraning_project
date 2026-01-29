#pragma once

#include "ErrorCode.h"
#include <string>

namespace yibo {

struct Error {
    ErrorCode code;
    std::string message;
    std::string details;

    Error(ErrorCode c, std::string msg = "", std::string det = "")
        : code(c)
        , message(msg.empty() ? std::string(ErrorCodeToString(c)) : std::move(msg))
        , details(std::move(det)) {}

    std::string ToString() const {
        std::string result = message;
        if (!details.empty()) {
            result += " (Details: " + details + ")";
        }
        return result;
    }
};

} // namespace yibo
