#pragma once

#include "Field.h"
#include <vector>
#include <string>

namespace yibo {

class Table {
protected:
    std::vector<Field> m_fields;

public:
    virtual ~Table() = default;

    // 纯虚函数：由子类实现
    virtual std::string GetTableName() const = 0;

    // 生成CREATE TABLE语句
    virtual std::string Create();

    // 生成INSERT语句
    std::string Insert();

    // 生成SELECT语句
    std::string Query(const std::string& condition = "");

    // 生成UPDATE语句
    std::string Modify(const std::string& condition);

    // 生成DELETE语句
    std::string Delete(const std::string& condition);

    // 访问字段
    std::vector<Field>& GetFields() { return m_fields; }
    const std::vector<Field>& GetFields() const { return m_fields; }

protected:
    virtual std::string GetSqlTypeName(SqlType type) const = 0;
};

} // namespace yibo
