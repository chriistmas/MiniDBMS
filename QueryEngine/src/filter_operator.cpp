#include "filter_operator.h"

FilterOperator::FilterOperator(std::unique_ptr<Operator> child, Predicate predicate)
    : child_(std::move(child)), predicate_(predicate) {}

void FilterOperator::Open() { child_->Open(); }

bool FilterOperator::Next(Record& out_record) {
    Record candidate;
    while (child_->Next(candidate)) {
        if (Evaluate(candidate)) {
            out_record = candidate;
            return true;
        }
    }
    return false;
}

void FilterOperator::Close() { child_->Close(); }

bool FilterOperator::Evaluate(const Record& record) const {
    if (predicate_.column_index < 0 || predicate_.column_index >= record.FieldCount()) {
        return false;
    }
    if (record.IsNull(predicate_.column_index)) {
        return false; // NULL nunca satisface una comparacion
    }

    if (predicate_.value.is_int) {
        int actual = record.GetInt(predicate_.column_index);
        int expected = predicate_.value.int_value;
        switch (predicate_.op) {
            case ComparisonOp::EQ: return actual == expected;
            case ComparisonOp::NE: return actual != expected;
            case ComparisonOp::LT: return actual < expected;
            case ComparisonOp::LE: return actual <= expected;
            case ComparisonOp::GT: return actual > expected;
            case ComparisonOp::GE: return actual >= expected;
        }
    } else {
        std::string actual = record.GetString(predicate_.column_index);
        const std::string& expected = predicate_.value.str_value;
        switch (predicate_.op) {
            case ComparisonOp::EQ: return actual == expected;
            case ComparisonOp::NE: return actual != expected;
            case ComparisonOp::LT: return actual < expected;
            case ComparisonOp::LE: return actual <= expected;
            case ComparisonOp::GT: return actual > expected;
            case ComparisonOp::GE: return actual >= expected;
        }
    }
    return false;
}
