#include "predicate.h"
#include <stdexcept>

ComparisonOp ParseComparisonOp(const std::string& token) {
    if (token == "=") return ComparisonOp::EQ;
    if (token == "!=" || token == "<>") return ComparisonOp::NE;
    if (token == "<") return ComparisonOp::LT;
    if (token == "<=") return ComparisonOp::LE;
    if (token == ">") return ComparisonOp::GT;
    if (token == ">=") return ComparisonOp::GE;
    throw std::runtime_error("Operador de comparacion no soportado: " + token);
}
