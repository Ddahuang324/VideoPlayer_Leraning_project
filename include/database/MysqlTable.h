#pragma once

#include "Table.h"

namespace yibo {

class MysqlTable : public Table {
protected:
    std::string GetSqlTypeName(SqlType type) const override;
};

} // namespace yibo
