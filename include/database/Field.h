#pragma once

#include "Types.h"
#include "common/Result.h"
#include "common/Error.h"
#include <string>
#include <string_view>
#include <optional>

namespace yibo {

class Field {
public:
    std::string name;
    SqlType type;
    unsigned attr;
    FieldValue value;
    std::optional<std::string> default_value;

    Field() : type(SqlType::TYPE_NULL), attr(FieldAttr::NONE) {}

    Field(std::string name, SqlType type, unsigned attr = FieldAttr::NONE)
        : name(std::move(name)), type(type), attr(attr), value(std::monostate{}) {}

    // 类型安全的值设置
    template<typename T>
    void SetValue(T&& val) {
        value = std::forward<T>(val);
    }

    // 从字符串加载值（零拷贝优化）
    Result<void, Error> LoadFromStr(std::string_view str);

    // 转换为SQL字符串
    std::string ToSqlString() const;

    // 判断是否为NULL
    bool IsNull() const {
        return std::holds_alternative<std::monostate>(value);
    }
};

} // namespace yibo
