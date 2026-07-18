#ifndef RECORD_H
#define RECORD_H

#include <string>
#include <vector>

/*
 * Record
 * -------------------------------------------------------------------------
 * Representa un registro/tupla generico como una lista de campos tipados.
 * Sabe serializarse a bytes (para guardarse dentro de una pagina) y
 * reconstruirse a partir de esos bytes.
 *
 * Formato de serializacion:
 *   [int field_count]
 *   por cada campo:
 *      [char tag]   'I' = INTEGER, 'S' = STRING
 *      si 'I': [int value]
 *      si 'S': [int length][bytes...]
 * -------------------------------------------------------------------------
 */
class Record {
public:
    Record() = default;

    void AddInt(int value);
    void AddString(const std::string& value);

    // Serializa el registro en out_buffer (debe tener espacio suficiente).
    // Devuelve el tamano en bytes ocupado.
    int Serialize(char* out_buffer) const;

    // Calcula cuantos bytes ocuparia el registro serializado (sin escribirlo).
    int SerializedSize() const;

    // Reconstruye un Record a partir de un buffer de bytes.
    static Record Deserialize(const char* buffer, int size);

    int FieldCount() const;
    int GetInt(int index) const;
    std::string GetString(int index) const;

    std::string ToString() const;

private:
    enum class FieldType { INT, STRING };

    struct Field {
        FieldType type;
        int int_value = 0;
        std::string str_value;
    };

    std::vector<Field> fields_;
};

#endif // RECORD_H
