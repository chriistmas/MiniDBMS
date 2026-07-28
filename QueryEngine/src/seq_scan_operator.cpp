#include "seq_scan_operator.h"

SeqScanOperator::SeqScanOperator(HeapFile& heap_file)
    : heap_file_(heap_file), iterator_(heap_file.GetIterator()) {}

void SeqScanOperator::Open() { iterator_.Open(); }

bool SeqScanOperator::Next(Record& out_record) {
    RID rid;
    return iterator_.Next(rid, out_record);
}

void SeqScanOperator::Close() { iterator_.Close(); }
