#include "project_operator.h"

ProjectOperator::ProjectOperator(std::unique_ptr<Operator> child,
                                  std::vector<int> column_indices,
                                  std::vector<ColumnDefinition> source_schema)
    : child_(std::move(child)),
      column_indices_(std::move(column_indices)),
      source_schema_(std::move(source_schema)) {}

void ProjectOperator::Open() { child_->Open(); }

bool ProjectOperator::Next(Record& out_record) {
    Record child_record;
    if (!child_->Next(child_record)) {
        return false;
    }

    out_record = Record();
    for (int idx : column_indices_) {
        if (idx < 0 || idx >= child_record.FieldCount()) {
            out_record.AddNull();
            continue;
        }
        if (child_record.IsNull(idx)) {
            out_record.AddNull();
            continue;
        }
        if (idx < static_cast<int>(source_schema_.size()) &&
            source_schema_[idx].type == "STRING") {
            out_record.AddString(child_record.GetString(idx));
        } else {
            out_record.AddInt(child_record.GetInt(idx));
        }
    }
    return true;
}

void ProjectOperator::Close() { child_->Close(); }
