#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

/*
 * Parser
 * -------------------------------------------------------------------------
 * Parser basico (no un SQL completo) para sentencias del tipo:
 *
 *   SELECT * FROM tabla
 *   SELECT col1, col2 FROM tabla
 *   SELECT col1, col2 FROM tabla WHERE columna = valor
 *   SELECT * FROM tabla WHERE columna > 10
 *   SELECT * FROM tabla WHERE nombre = 'Ana'
 *
 * Operadores de comparacion soportados: = != <> < <= > >=
 * Valores: enteros sin comillas, o cadenas entre comillas simples.
 * -------------------------------------------------------------------------
 */
struct ParsedWhere {
    std::string column;
    std::string op;         // "=", "!=", "<", "<=", ">", ">="
    std::string value_text; // valor tal cual aparecio (sin comillas)
    bool is_string = false; // true si el valor estaba entre comillas
};

struct ParsedQuery {
    std::vector<std::string> columns; // {"*"} significa todas las columnas
    std::string table;
    bool has_where = false;
    ParsedWhere where;
};

class Parser {
public:
    // Parsea la sentencia. Lanza std::runtime_error con un mensaje
    // descriptivo si la sintaxis no es valida.
    static ParsedQuery Parse(const std::string& sql);

private:
    static std::vector<std::string> Tokenize(const std::string& sql);
};

#endif // PARSER_H
