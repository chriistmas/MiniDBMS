#ifndef FILTER_OPERATOR_H
#define FILTER_OPERATOR_H

#include <memory>
#include "operator.h"
#include "predicate.h"

/*
 * FilterOperator
 * -------------------------------------------------------------------------
 * Operador fisico que envuelve a otro operador (su hijo) y solo deja pasar
 * los registros que cumplen un Predicate (implementa la clausula WHERE).
 * En cada Next() consume registros del hijo hasta encontrar uno que
 * cumpla la condicion, o hasta agotar al hijo.
 * -------------------------------------------------------------------------
 */
class FilterOperator : public Operator {
public:
    FilterOperator(std::unique_ptr<Operator> child, Predicate predicate);

    void Open() override;
    bool Next(Record& out_record) override;
    void Close() override;

private:
    std::unique_ptr<Operator> child_;
    Predicate predicate_;

    bool Evaluate(const Record& record) const;
};

#endif // FILTER_OPERATOR_H
