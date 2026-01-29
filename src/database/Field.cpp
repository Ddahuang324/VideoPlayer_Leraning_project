#include "database/Field.h"
#include "common/ErrorCode.h"
#include <stdexcept>

namespace yibo {

Result<void, Error> Field::LoadFromStr(std::string_view str) {
    if (str.empty() && !(attr & NOT_NULL)) {//str为空或者未设置非空约束
        value = std::monostate{};
        return Result<void, Error>::Ok();
    }

    try {
        switch (type) {
            case SqlType::TYPE_BOOL:
                value = (str == "1" || str == "true");
                break;
            case SqlType::TYPE_INT:
                value = std::stoi(std::string(str));
                break;
            case SqlType::TYPE_REAL:
                value = std::stod(std::string(str));
                break;
            case SqlType::TYPE_VARCHAR:
            case SqlType::TYPE_TEXT:
            case SqlType::TYPE_BLOB:
            case SqlType::TYPE_DATETIME:
                value = std::string(str);
                break;
            default:
                value = std::monostate{};
        }
        return Result<void, Error>::Ok();
    } catch (const std::exception& e) {
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseError, std::string("Type conversion failed: ") + e.what())
        );
    }
}

std::string Field::ToSqlString() const {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return "NULL";
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "1" : "0";
        } else if constexpr (std::is_same_v<T, int>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return "'" + arg + "'";
        }
        return "NULL";
    }, value);
}

} // namespace yibo
