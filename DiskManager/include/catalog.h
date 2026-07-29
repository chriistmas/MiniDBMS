#ifndef CATALOG_H
#define CATALOG_H

#include <string>
#include <vector>

/*
 * Catalog
 * -------------------------------------------------------------------------
 * Guarda el esquema de la base de datos: nombre de la base de datos,
 * tablas, columnas y su tipo, y en que pagina inicia el HeapFile de
 * cada tabla. Es la informacion administrativa que normalmente vive
 * en la Pagina 0 del archivo .db.
 * -------------------------------------------------------------------------
 */
struct ColumnDefinition {
    std::string name;
    std::string type; // "INTEGER" o "STRING"
};

struct TableDefinition {
    std::string table_name;
    std::vector<ColumnDefinition> columns;
    int first_page_id = -1; // primera pagina del HeapFile de esta tabla
    std::vector<int> page_ids; // TODAS las paginas de la tabla (para reabrir correctamente)
};

class Catalog {
public:
    void SetDatabaseName(const std::string& name);
    const std::string& GetDatabaseName() const;

    void AddTable(const TableDefinition& table);
    TableDefinition* FindTable(const std::string& table_name);

    std::vector<TableDefinition>& GetTables();

    // Convierte el catalogo completo en texto plano (formato simple, legible)
    std::string Serialize() const;

    // Reconstruye un Catalog a partir de su representacion en texto
    static Catalog Deserialize(const std::string& text);

    void Print() const;

private:
    std::string database_name_;
    std::vector<TableDefinition> tables_;
};

#endif // CATALOG_H
