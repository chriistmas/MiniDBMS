#include "parser.h"
#include <cctype>
#include <stdexcept>
#include <algorithm>

namespace {

std::string ToUpper(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                    [](unsigned char c) { return std::toupper(c); });
    return out;
}

bool IsOperatorChar(char c) { return c == '=' || c == '!' || c == '<' || c == '>'; }

bool LooksLikeInteger(const std::string& s) {
    if (s.empty()) return false;
    size_t i = (s[0] == '-') ? 1 : 0;
    if (i >= s.size()) return false;
    for (; i < s.size(); i++) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

} // namespace

std::vector<std::string> Parser::Tokenize(const std::string& sql) {
    std::vector<std::string> tokens;
    size_t i = 0;
    size_t n = sql.size();

    while (i < n) {
        char c = sql[i];

        if (std::isspace(static_cast<unsigned char>(c))) {
            i++;
            continue;
        }

        if (c == ',') {
            tokens.push_back(",");
            i++;
            continue;
        }

        if (c == '\'') { // cadena entre comillas simples
            size_t j = i + 1;
            std::string value;
            while (j < n && sql[j] != '\'') {
                value += sql[j];
                j++;
            }
            if (j >= n) {
                throw std::runtime_error("Cadena sin cerrar (falta comilla simple)");
            }
            tokens.push_back("'" + value + "'"); // se conservan las comillas como marca
            i = j + 1;
            continue;
        }

        if (IsOperatorChar(c)) {
            std::string op(1, c);
            if (i + 1 < n && sql[i + 1] == '=') {
                op += '=';
                i += 2;
            } else if (c == '<' && i + 1 < n && sql[i + 1] == '>') {
                op = "<>";
                i += 2;
            } else {
                i += 1;
            }
            tokens.push_back(op);
            continue;
        }

        // palabra / identificador / numero
        size_t j = i;
        while (j < n && !std::isspace(static_cast<unsigned char>(sql[j])) &&
               sql[j] != ',' && !IsOperatorChar(sql[j]) && sql[j] != '\'') {
            j++;
        }
        tokens.push_back(sql.substr(i, j - i));
        i = j;
    }
    return tokens;
}

ParsedQuery Parser::Parse(const std::string& sql) {
    std::vector<std::string> tok = Tokenize(sql);
    size_t pos = 0;

    auto expect_keyword = [&](const std::string& kw) {
        if (pos >= tok.size() || ToUpper(tok[pos]) != kw) {
            throw std::runtime_error("Se esperaba '" + kw + "' en la sentencia");
        }
        pos++;
    };
    auto peek = [&]() -> std::string { return pos < tok.size() ? tok[pos] : std::string(); };

    ParsedQuery q;

    expect_keyword("SELECT");

    // Lista de columnas
    if (peek() == "*") {
        q.columns.push_back("*");
        pos++;
    } else {
        while (true) {
            if (pos >= tok.size()) throw std::runtime_error("Se esperaba una columna");
            q.columns.push_back(tok[pos++]);
            if (peek() == ",") {
                pos++;
                continue;
            }
            break;
        }
    }

    expect_keyword("FROM");
    if (pos >= tok.size()) throw std::runtime_error("Se esperaba el nombre de la tabla");
    q.table = tok[pos++];

    if (pos < tok.size() && ToUpper(tok[pos]) == "WHERE") {
        pos++;
        if (pos >= tok.size()) throw std::runtime_error("WHERE incompleto: falta columna");
        q.where.column = tok[pos++];

        if (pos >= tok.size()) throw std::runtime_error("WHERE incompleto: falta operador");
        q.where.op = tok[pos++];

        if (pos >= tok.size()) throw std::runtime_error("WHERE incompleto: falta valor");
        std::string raw = tok[pos++];
        if (raw.size() >= 2 && raw.front() == '\'' && raw.back() == '\'') {
            q.where.value_text = raw.substr(1, raw.size() - 2);
            q.where.is_string = true;
        } else if (LooksLikeInteger(raw)) {
            q.where.value_text = raw;
            q.where.is_string = false;
        } else {
            // Palabra suelta sin comillas: se trata como string igualmente
            q.where.value_text = raw;
            q.where.is_string = true;
        }
        q.has_where = true;
    }

    if (pos != tok.size()) {
        throw std::runtime_error("Tokens inesperados al final de la sentencia: '" + tok[pos] + "'");
    }

    return q;
}
