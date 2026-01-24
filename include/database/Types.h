#pragma once

#include <variant>
#include <string>

namespace yibo {

// SQL类型枚举
enum class SqlType {
    TYPE_NULL = 0,
    TYPE_BOOL,
    TYPE_INT,
    TYPE_DATETIME,
    TYPE_REAL,
    TYPE_VARCHAR,
    TYPE_TEXT,
    TYPE_BLOB
};

// 字段属性枚举（使用位标志）
enum FieldAttr : unsigned {
    NONE = 0,
    NOT_NULL = 1 << 0,
    DEFAULT = 1 << 1,
    UNIQUE = 1 << 2,
    PRIMARY_KEY = 1 << 3,
    CHECK = 1 << 4,
    AUTOINCREMENT = 1 << 5
};

// 字段值类型：使用variant替代unsafe union
using FieldValue = std::variant<
    std::monostate,  // NULL
    bool,            // BOOL
    int,             // INT
    double,          // REAL
    std::string      // VARCHAR/TEXT/BLOB
>;

} // namespace yibo
