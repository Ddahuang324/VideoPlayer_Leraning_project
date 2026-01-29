#include "database/Table.h"

namespace yibo {

std::string Table::Create() {
    std::string sql = "CREATE TABLE IF NOT EXISTS " + GetTableName() + " (";

    for (size_t i = 0; i < m_fields.size(); ++i) {
        const auto& field = m_fields[i];
        sql += field.name + " ";
        sql += GetSqlTypeName(field.type);

        if (field.attr & PRIMARY_KEY) sql += " PRIMARY KEY";
        if (field.attr & AUTOINCREMENT) sql += " AUTOINCREMENT";
        if (field.attr & NOT_NULL) sql += " NOT NULL";
        if (field.attr & UNIQUE) sql += " UNIQUE";
        if (field.default_value) sql += " DEFAULT " + *field.default_value;

        if (i < m_fields.size() - 1) sql += ", ";
    }

    sql += ")";
    return sql;
}

std::string Table::Insert() {
    std::string sql = "INSERT INTO " + GetTableName() + " (";
    std::string values = " VALUES (";

    for (size_t i = 0; i < m_fields.size(); ++i) {
        sql += m_fields[i].name;
        values += m_fields[i].ToSqlString();

        if (i < m_fields.size() - 1) {
            sql += ", ";
            values += ", ";
        }
    }

    sql += ")" + values + ")";
    return sql;
}

std::string Table::Query(const std::string& condition) {
    std::string sql = "SELECT * FROM " + GetTableName();
    if (!condition.empty()) {
        sql += " WHERE " + condition;
    }
    return sql;
}

std::string Table::Modify(const std::string& condition) {
    std::string sql = "UPDATE " + GetTableName() + " SET ";

    bool first = true;
    for (size_t i = 0; i < m_fields.size(); ++i) {
        if (m_fields[i].attr & PRIMARY_KEY) continue;
        if (!first) sql += ", ";
        sql += m_fields[i].name + " = " + m_fields[i].ToSqlString();
        first = false;
    }

    sql += " WHERE " + condition;
    return sql;
}

std::string Table::Delete(const std::string& condition) {
    return "DELETE FROM " + GetTableName() + " WHERE " + condition;
}

} // namespace yibo
