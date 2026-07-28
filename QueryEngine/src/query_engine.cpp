#include "query_engine.h"
#include "seq_scan_operator.h"
#include "filter_operator.h"
#include "project_operator.h"
#include "predicate.h"
#include <iostream>
#include <stdexcept>

QueryEngine::QueryEngine(Catalog& catalog) : catalog_(catalog) {}

void QueryEngine::RegisterTable(const std::string& table_name, HeapFile* heap_file) {
    tables_[table_name] = heap_file;
}

void QueryEngine::RegisterEqualityIndex(const std::string& table_name,
                                         const std::string& column_name,
                                         IndexScanOperator::SearchFn search_fn) {
    indexes_[table_name] = IndexInfo{column_name, std::move(search_fn)};
}

std::unique_ptr<Operator> QueryEngine::BuildPlan(const ParsedQuery& query,
                                                  const TableDefinition& table_def,
                                                  std::vector<int>& out_projected_indices) const {
    HeapFile* heap_file = tables_.at(query.table);

    auto find_col_index = [&](const std::string& name) -> int {
        for (size_t i = 0; i < table_def.columns.size(); i++) {
            if (table_def.columns[i].name == name) return static_cast<int>(i);
        }
        throw std::runtime_error("La columna '" + name + "' no existe en la tabla '" +
                                  table_def.table_name + "'");
    };

    std::unique_ptr<Operator> base;
    bool used_index = false;

    // Planner: WHERE de igualdad sobre una columna INTEGER con indice
    // registrado -> IndexScanOperator (evita el scan completo).
    if (query.has_where && (query.where.op == "=") && !query.where.is_string) {
        auto idx_it = indexes_.find(query.table);
        if (idx_it != indexes_.end() && idx_it->second.column == query.where.column) {
            int key = std::stoi(query.where.value_text);
            base = std::make_unique<IndexScanOperator>(*heap_file, idx_it->second.search_fn, key);
            used_index = true;
        }
    }

    if (!used_index) {
        base = std::make_unique<SeqScanOperator>(*heap_file);
        if (query.has_where) {
            int col_idx = find_col_index(query.where.column);

            Predicate pred;
            pred.column_index = col_idx;
            pred.op = ParseComparisonOp(query.where.op);
            pred.value.is_int = !query.where.is_string;
            if (pred.value.is_int) {
                pred.value.int_value = std::stoi(query.where.value_text);
            } else {
                pred.value.str_value = query.where.value_text;
            }
            base = std::make_unique<FilterOperator>(std::move(base), pred);
        }
    }

    // Resolver la lista de columnas del SELECT a indices del esquema.
    std::vector<int> proj_indices;
    if (query.columns.size() == 1 && query.columns[0] == "*") {
        for (size_t i = 0; i < table_def.columns.size(); i++) {
            proj_indices.push_back(static_cast<int>(i));
        }
    } else {
        for (const auto& col_name : query.columns) {
            proj_indices.push_back(find_col_index(col_name));
        }
    }
    out_projected_indices = proj_indices;

    return std::make_unique<ProjectOperator>(std::move(base), proj_indices, table_def.columns);
}

int QueryEngine::Execute(const std::string& sql) const {
    ParsedQuery query;
    try {
        query = Parser::Parse(sql);
    } catch (const std::exception& e) {
        std::cout << "[Error de sintaxis] " << e.what() << "\n";
        return -1;
    }

    if (tables_.find(query.table) == tables_.end()) {
        std::cout << "[Error] Tabla no registrada en el motor de consultas: " << query.table << "\n";
        return -1;
    }
    TableDefinition* table_def = catalog_.FindTable(query.table);
    if (table_def == nullptr) {
        std::cout << "[Error] Tabla no existe en el catalogo: " << query.table << "\n";
        return -1;
    }

    std::vector<int> projected_indices;
    std::unique_ptr<Operator> plan;
    try {
        plan = BuildPlan(query, *table_def, projected_indices);
    } catch (const std::exception& e) {
        std::cout << "[Error de planificacion] " << e.what() << "\n";
        return -1;
    }

    std::cout << "  ";
    for (size_t i = 0; i < projected_indices.size(); i++) {
        std::cout << table_def->columns[projected_indices[i]].name;
        if (i + 1 < projected_indices.size()) std::cout << " | ";
    }
    std::cout << "\n  " << std::string(48, '-') << "\n";

    plan->Open();
    Record row;
    int count = 0;
    while (plan->Next(row)) {
        std::cout << "  ";
        for (int i = 0; i < row.FieldCount(); i++) {
            if (row.IsNull(i)) {
                std::cout << "NULL";
            } else if (i < static_cast<int>(projected_indices.size()) &&
                       table_def->columns[projected_indices[i]].type == "STRING") {
                std::cout << row.GetString(i);
            } else {
                std::cout << row.GetInt(i);
            }
            if (i + 1 < row.FieldCount()) std::cout << " | ";
        }
        std::cout << "\n";
        count++;
    }
    plan->Close();

    std::cout << "  (" << count << " fila(s))\n";
    return count;
}
