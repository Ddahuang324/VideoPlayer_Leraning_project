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
    TYPE_REAL, //对应cpp中的浮点数
    TYPE_VARCHAR, //对应cpp中的字符串
    TYPE_TEXT,//对应cpp中的字符串
    TYPE_BLOB//对应cpp中的字符串，二进制大对象
};

// 字段属性枚举（使用位标志）
enum FieldAttr : unsigned {
    NONE = 0,
    NOT_NULL = 1 << 0,      // 非空
    DEFAULT = 1 << 1,       // 有默认值
    UNIQUE = 1 << 2,        // 唯一约束
    PRIMARY_KEY = 1 << 3,   // 主键
    CHECK = 1 << 4,         // 检查约束
    AUTOINCREMENT = 1 << 5  // 自增
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
