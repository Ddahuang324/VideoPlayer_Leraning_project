#pragma once

#include "database/MysqlTable.h"

namespace yibo {

class UserTable : public MysqlTable {
public:
    std::string GetTableName() const override { return "users"; }
};

} // namespace yibo
