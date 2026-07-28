#ifndef OPERATOR_H
#define OPERATOR_H

#include "record.h"

/*
 * Operator
 * -------------------------------------------------------------------------
 * Interfaz comun de todo operador fisico del motor de consultas, siguiendo
 * el Modelo Volcano (Iterator Model):
 *
 *   Open()  -> inicializa el operador y (recursivamente) sus hijos.
 *   Next()  -> produce EL SIGUIENTE registro de salida, o false si ya no
 *              quedan mas (agota el operador). No materializa resultados
 *              intermedios completos: cada llamada "hala" (pull) un tuple
 *              a la vez desde sus operadores hijos.
 *   Close() -> libera recursos y cierra (recursivamente) a sus hijos.
 *
 * Los operadores se componen en un arbol (plan fisico). Por ejemplo, para
 * "SELECT nombre FROM alumnos WHERE edad > 18":
 *
 *      ProjectOperator(nombre)
 *              |
 *      FilterOperator(edad > 18)
 *              |
 *      SeqScanOperator(alumnos)
 * -------------------------------------------------------------------------
 */
class Operator {
public:
    virtual ~Operator() = default;

    virtual void Open() = 0;
    virtual bool Next(Record& out_record) = 0;
    virtual void Close() = 0;
};

#endif // OPERATOR_H
