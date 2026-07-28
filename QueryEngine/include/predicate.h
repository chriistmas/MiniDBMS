#ifndef PREDICATE_H
#define PREDICATE_H

#include <string>

/*
 * Predicate
 * -------------------------------------------------------------------------
 * Representa una condicion simple "columna OP valor" (una clausula WHERE
 * de un solo termino), ya resuelta contra el esquema de la tabla:
 * column_index es la posicion del campo dentro del Record, y value ya
 * viene tipado segun la columna (INTEGER o STRING).
 * -------------------------------------------------------------------------
 */
enum class ComparisonOp { EQ, NE, LT, LE, GT, GE };

struct PredicateValue {
    bool is_int = true;
    int int_value = 0;
    std::string str_value;
};

struct Predicate {
    int column_index = -1;
    ComparisonOp op = ComparisonOp::EQ;
    PredicateValue value;
};

ComparisonOp ParseComparisonOp(const std::string& token);

#endif // PREDICATE_H
