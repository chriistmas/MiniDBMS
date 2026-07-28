#ifndef INDEX_SCAN_OPERATOR_H
#define INDEX_SCAN_OPERATOR_H

#include <functional>
#include <vector>
#include "operator.h"
#include "heap_file.h"

/*
 * IndexScanOperator
 * -------------------------------------------------------------------------
 * Operador fisico hoja alternativo a SeqScanOperator: en vez de recorrer
 * TODA la tabla, usa un indice (BPlusTree::Search o StaticHashIndex::Search,
 * ambos con la misma firma) para obtener directamente los RID que cumplen
 * "columna = valor", y luego recupera cada registro por RID desde el
 * HeapFile. Se usa cuando el planner (QueryEngine::BuildPlan) detecta un
 * WHERE de igualdad sobre una columna indexada.
 *
 * Se recibe el metodo Search como std::function para no acoplar este
 * operador a una implementacion concreta de indice.
 * -------------------------------------------------------------------------
 */
class IndexScanOperator : public Operator {
public:
    using SearchFn = std::function<bool(int key, std::vector<RID>& out_rids)>;

    IndexScanOperator(HeapFile& heap_file, SearchFn search_fn, int key);

    void Open() override;
    bool Next(Record& out_record) override;
    void Close() override;

private:
    HeapFile& heap_file_;
    SearchFn search_fn_;
    int key_;
    std::vector<RID> matches_;
    size_t pos_ = 0;
};

#endif // INDEX_SCAN_OPERATOR_H
