#include "catalog.h"
#include <sstream>
#include <iostream>

void Catalog::SetDatabaseName(const std::string& name) { database_name_ = name; }
const std::string& Catalog::GetDatabaseName() const { return database_name_; }

void Catalog::AddTable(const TableDefinition& table) { tables_.push_back(table); }

TableDefinition* Catalog::FindTable(const std::string& table_name) {
    for (auto& t : tables_) {
        if (t.table_name == table_name) return &t;
    }
    return nullptr;
}

std::vector<TableDefinition>& Catalog::GetTables() { return tables_; }

std::string Catalog::Serialize() const {
    std::ostringstream oss;
    oss << "DATABASE " << database_name_ << "\n";
    for (const auto& table : tables_) {
        oss << "TABLE " << table.table_name << " " << table.first_page_id << "\n";
        oss << "PAGES";
        for (int pid : table.page_ids) oss << " " << pid;
        if (table.page_ids.empty()) oss << " " << table.first_page_id;
        oss << "\n";
        for (const auto& col : table.columns) {
            oss << "COLUMN " << col.name << " " << col.type << "\n";
        }
        oss << "ENDTABLE\n";
    }
    return oss.str();
}

Catalog Catalog::Deserialize(const std::string& text) {
    Catalog catalog;
    std::istringstream iss(text);
    std::string line;
    TableDefinition current_table;
    bool in_table = false;

    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        std::istringstream ls(line);
        std::string keyword;
        ls >> keyword;

        if (keyword == "DATABASE") {
            std::string name;
            ls >> name;
            catalog.SetDatabaseName(name);
        } else if (keyword == "TABLE") {
            current_table = TableDefinition();
            ls >> current_table.table_name >> current_table.first_page_id;
            in_table = true;
        } else if (keyword == "PAGES" && in_table) {
            int pid;
            while (ls >> pid) current_table.page_ids.push_back(pid);
        } else if (keyword == "COLUMN" && in_table) {
            ColumnDefinition col;
            ls >> col.name >> col.type;
            current_table.columns.push_back(col);
        } else if (keyword == "ENDTABLE" && in_table) {
            catalog.AddTable(current_table);
            in_table = false;
        }
    }
    return catalog;
}

void Catalog::Print() const {
    std::cout << "Catalogo de la base de datos: " << database_name_ << "\n";
    for (const auto& table : tables_) {
        std::cout << "  Tabla: " << table.table_name
                  << " (primera pagina: " << table.first_page_id << ")\n";
        for (const auto& col : table.columns) {
            std::cout << "    - " << col.name << " : " << col.type << "\n";
        }
    }
}
