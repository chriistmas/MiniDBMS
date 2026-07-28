#ifndef PROJECT_OPERATOR_H
#define PROJECT_OPERATOR_H

#include <memory>
#include <vector>
#include "operator.h"
#include "catalog.h"

/*
 * ProjectOperator
 * -------------------------------------------------------------------------
 * Operador fisico que envuelve a otro operador (su hijo) y devuelve solo
 * un subconjunto de columnas (implementa la lista de columnas del SELECT).
 * Necesita el esquema completo de la tabla de origen para saber el TIPO
 * (INTEGER/STRING) de cada columna que copia, ya que Record no expone el
 * tipo de un campo por si solo.
 * -------------------------------------------------------------------------
 */
class ProjectOperator : public Operator {
public:
    ProjectOperator(std::unique_ptr<Operator> child,
                     std::vector<int> column_indices,
                     std::vector<ColumnDefinition> source_schema);

    void Open() override;
    bool Next(Record& out_record) override;
    void Close() override;

private:
    std::unique_ptr<Operator> child_;
    std::vector<int> column_indices_;
    std::vector<ColumnDefinition> source_schema_;
};

#endif // PROJECT_OPERATOR_H
