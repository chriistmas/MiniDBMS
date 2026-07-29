#include "csv_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>

std::vector<std::string> CsvLoader::SplitCsvLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // Quitar espacios / retorno de carro sobrantes
        while (!token.empty() && (token.back() == '\r' || token.back() == ' ')) {
            token.pop_back();
        }
        tokens.push_back(token);
    }
    return tokens;
}

bool CsvLoader::LooksLikeInteger(const std::string& token) {
    if (token.empty()) return false;
    size_t start = (token[0] == '-') ? 1 : 0;
    if (start >= token.size()) return false;
    for (size_t i = start; i < token.size(); i++) {
        if (!std::isdigit(static_cast<unsigned char>(token[i]))) return false;
    }
    return true;
}

bool CsvLoader::LoadCsvIntoTable(const std::string& csv_path,
                                  const std::string& table_name,
                                  IPageStore& page_store,
                                  Catalog& catalog,
                                  HeapFile** out_heap_file) {
    std::ifstream file(csv_path);
    if (!file.is_open()) {
        std::cerr << "Error: no se pudo abrir el archivo CSV: " << csv_path << "\n";
        return false;
    }

    std::string header_line;
    if (!std::getline(file, header_line)) {
        std::cerr << "Error: el CSV esta vacio.\n";
        return false;
    }
    std::vector<std::string> column_names = SplitCsvLine(header_line);

    std::string first_data_line;
    if (!std::getline(file, first_data_line)) {
        std::cerr << "Error: el CSV no tiene filas de datos para inferir tipos.\n";
        return false;
    }
    std::vector<std::string> first_row = SplitCsvLine(first_data_line);

    // 1. Crear esquema (inferencia simple de tipos con la primera fila)
    TableDefinition table;
    table.table_name = table_name;
    for (size_t i = 0; i < column_names.size(); i++) {
        ColumnDefinition col;
        col.name = column_names[i];
        col.type = (i < first_row.size() && LooksLikeInteger(first_row[i]))
                       ? "INTEGER" : "STRING";
        table.columns.push_back(col);
    }

    // 2. Crear el archivo/paginas fisicas para esta tabla (primera pagina)
    int first_page_id = page_store.AllocatePage();
    table.first_page_id = first_page_id;
    table.page_ids.push_back(first_page_id);
    catalog.AddTable(table);

    HeapFile* heap_file = new HeapFile(page_store, first_page_id);

    // 3. Convertir la primera fila (ya leida) y las restantes en registros
    auto insert_row = [&](const std::vector<std::string>& row) {
        Record record;
        for (size_t i = 0; i < table.columns.size(); i++) {
            std::string value = (i < row.size()) ? row[i] : "";
            if (table.columns[i].type == "INTEGER") {
                record.AddInt(value.empty() ? 0 : std::stoi(value));
            } else {
                record.AddString(value);
            }
        }
        heap_file->InsertRecord(record);
    };

    insert_row(first_row);

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<std::string> row = SplitCsvLine(line);
        insert_row(row);
    }

    file.close();

    std::cout << "CSV '" << csv_path << "' cargado en la tabla '" << table_name
              << "' (primera pagina: " << first_page_id << ").\n";

    *out_heap_file = heap_file;
    return true;
}
