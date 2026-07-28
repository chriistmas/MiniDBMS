#include "index_scan_operator.h"

IndexScanOperator::IndexScanOperator(HeapFile& heap_file, SearchFn search_fn, int key)
    : heap_file_(heap_file), search_fn_(std::move(search_fn)), key_(key) {}

void IndexScanOperator::Open() {
    matches_.clear();
    pos_ = 0;
    search_fn_(key_, matches_);
}

bool IndexScanOperator::Next(Record& out_record) {
    while (pos_ < matches_.size()) {
        const RID& rid = matches_[pos_++];
        if (heap_file_.GetRecord(rid, out_record)) {
            return true;
        }
    }
    return false;
}

void IndexScanOperator::Close() {
    matches_.clear();
    pos_ = 0;
}
