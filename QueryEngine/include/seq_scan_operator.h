#ifndef SEQ_SCAN_OPERATOR_H
#define SEQ_SCAN_OPERATOR_H

#include "operator.h"
#include "heap_file.h"

/*
 * SeqScanOperator
 * -------------------------------------------------------------------------
 * Operador fisico hoja: recorre TODOS los registros activos de un HeapFile
 * en su orden fisico, usando HeapFile::Iterator. Es el operador de acceso
 * mas basico (equivalente a un "table scan" / "full scan").
 * -------------------------------------------------------------------------
 */
class SeqScanOperator : public Operator {
public:
    explicit SeqScanOperator(HeapFile& heap_file);

    void Open() override;
    bool Next(Record& out_record) override;
    void Close() override;

private:
    HeapFile& heap_file_;
    HeapFile::Iterator iterator_;
};

#endif // SEQ_SCAN_OPERATOR_H
