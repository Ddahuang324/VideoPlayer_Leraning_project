#include "database/Sqlite3Table.h"

namespace yibo {

std::string Sqlite3Table::GetSqlTypeName(SqlType type) const {
    switch (type) {
        case SqlType::TYPE_BOOL:
        case SqlType::TYPE_INT: return "INTEGER";
        case SqlType::TYPE_DATETIME: return "TEXT";
        case SqlType::TYPE_REAL: return "REAL";
        case SqlType::TYPE_VARCHAR:
        case SqlType::TYPE_TEXT: return "TEXT";
        case SqlType::TYPE_BLOB: return "BLOB";
        default: return "TEXT";
    }
}

} // namespace yibo
