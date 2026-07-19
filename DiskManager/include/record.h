#ifndef RECORD_H
#define RECORD_H

#include <string>
#include <vector>

/*
 * Record
 * -------------------------------------------------------------------------
 * Representa un registro/tupla generico como una lista de campos tipados.
 *
 * Formato de serializacion (Slotted Record Format):
 *   [int field_count]
 *   [Null Bitmap] -> ceil(field_count / 8.0) bytes
 *   [Directorio de longitud fija] -> Por cada campo (9 bytes):
 *       [char tag] 'I' = INTEGER, 'S' = STRING
 *       [int offset_or_val] Si 'I' es el valor, si 'S' es el offset al dato variable
 *       [int length] Si 'S' es la longitud del string, si 'I' es 0.
 *   [Datos de longitud variable] -> Los strings concatenados
 * -------------------------------------------------------------------------
 */
class Record {
public:
    Record() = default;

    void AddInt(int value);
    void AddString(const std::string& value);
    void AddNull(); // Agrega un campo nulo (sin tipo especifico para simplificar)

    // Serializa el registro en out_buffer (debe tener espacio suficiente).
    // Devuelve el tamano en bytes ocupado.
    int Serialize(char* out_buffer) const;

    // Calcula cuantos bytes ocuparia el registro serializado (sin escribirlo).
    int SerializedSize() const;

    // Reconstruye un Record a partir de un buffer de bytes.
    static Record Deserialize(const char* buffer, int size);

    int FieldCount() const;
    bool IsNull(int index) const;
    int GetInt(int index) const;
    std::string GetString(int index) const;

    std::string ToString() const;

private:
    enum class FieldType { INT, STRING, NULL_TYPE };

    struct Field {
        FieldType type;
        bool is_null = false;
        int int_value = 0;
        std::string str_value;
    };

    std::vector<Field> fields_;
};

#endif // RECORD_H
