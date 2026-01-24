#include "database/MysqlTable.h"

namespace yibo {

std::string MysqlTable::GetSqlTypeName(SqlType type) const {
    switch (type) {
        case SqlType::TYPE_BOOL: return "TINYINT(1)";
        case SqlType::TYPE_INT: return "INTEGER";
        case SqlType::TYPE_DATETIME: return "DATETIME";
        case SqlType::TYPE_REAL: return "DOUBLE";
        case SqlType::TYPE_VARCHAR: return "VARCHAR(255)";
        case SqlType::TYPE_TEXT: return "TEXT";
        case SqlType::TYPE_BLOB: return "BLOB";
        default: return "TEXT";
    }
}

} // namespace yibo
