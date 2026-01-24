#pragma once

#include "Table.h"

namespace yibo {

class Sqlite3Table : public Table {
protected:
    std::string GetSqlTypeName(SqlType type) const override;
};

} // namespace yibo
