#ifndef QUERY_ENGINE_H
#define QUERY_ENGINE_H

#include <memory>
#include <string>
#include <unordered_map>
#include "catalog.h"
#include "heap_file.h"
#include "operator.h"
#include "parser.h"
#include "index_scan_operator.h"

/*
 * QueryEngine
 * -------------------------------------------------------------------------
 * Capa que conecta: Parser (texto -> ParsedQuery) -> Planner (ParsedQuery
 * -> arbol de operadores fisicos) -> Ejecucion (Volcano: Open/Next/Close).
 *
 *   "SELECT ..."  --Parser-->  ParsedQuery  --BuildPlan-->  Operator*
 *                                                                 |
 *                                              Open() / Next() / Close()
 *
 * Planner (basico, heuristico):
 *   - Si hay WHERE con '=' sobre una columna que tiene un indice
 *     registrado (RegisterEqualityIndex) -> IndexScanOperator.
 *   - En cualquier otro caso -> SeqScanOperator, envuelto en
 *     FilterOperator si hay WHERE.
 *   - Siempre se envuelve el resultado en un ProjectOperator segun las
 *     columnas pedidas en el SELECT.
 * -------------------------------------------------------------------------
 */
class QueryEngine {
public:
    explicit QueryEngine(Catalog& catalog);

    // Asocia el nombre logico de una tabla (del catalogo) con su HeapFile.
    void RegisterTable(const std::string& table_name, HeapFile* heap_file);

    // Registra un indice de igualdad (BPlusTree::Search o
    // StaticHashIndex::Search) disponible para una columna INTEGER de una
    // tabla, para que el planner pueda usar IndexScan en vez de SeqScan.
    void RegisterEqualityIndex(const std::string& table_name,
                                const std::string& column_name,
                                IndexScanOperator::SearchFn search_fn);

    // Parsea, planifica, ejecuta e imprime la sentencia. Devuelve la
    // cantidad de filas resultantes (-1 si hubo un error).
    int Execute(const std::string& sql) const;

private:
    Catalog& catalog_;
    std::unordered_map<std::string, HeapFile*> tables_;

    struct IndexInfo {
        std::string column;
        IndexScanOperator::SearchFn search_fn;
    };
    std::unordered_map<std::string, IndexInfo> indexes_; // clave = tabla

    std::unique_ptr<Operator> BuildPlan(const ParsedQuery& query,
                                         const TableDefinition& table_def,
                                         std::vector<int>& out_projected_indices) const;
};

#endif // QUERY_ENGINE_H
