#include "record.h"
#include <cstring>
#include <sstream>

void Record::AddInt(int value) {
    Field f;
    f.type = FieldType::INT;
    f.int_value = value;
    fields_.push_back(f);
}

void Record::AddString(const std::string& value) {
    Field f;
    f.type = FieldType::STRING;
    f.str_value = value;
    fields_.push_back(f);
}

void Record::AddNull() {
    Field f;
    f.type = FieldType::NULL_TYPE;
    f.is_null = true;
    fields_.push_back(f);
}

int Record::SerializedSize() const {
    int field_count = static_cast<int>(fields_.size());
    int size = static_cast<int>(sizeof(int)); // field_count

    int bitmap_size = (field_count + 7) / 8;
    size += bitmap_size;

    int fixed_dir_size = field_count * (1 + sizeof(int) + sizeof(int)); // tag + offset_or_val + length
    size += fixed_dir_size;

    for (const auto& f : fields_) {
        if (!f.is_null && f.type == FieldType::STRING) {
            size += static_cast<int>(f.str_value.size());
        }
    }
    return size;
}

int Record::Serialize(char* out_buffer) const {
    int offset = 0;
    int field_count = static_cast<int>(fields_.size());
    
    // 1. Escribir field_count
    std::memcpy(out_buffer + offset, &field_count, sizeof(int));
    offset += sizeof(int);

    // 2. Crear y escribir Null Bitmap
    int bitmap_size = (field_count + 7) / 8;
    std::vector<char> bitmap(bitmap_size, 0);
    for (int i = 0; i < field_count; i++) {
        if (fields_[i].is_null) {
            int byte_idx = i / 8;
            int bit_idx = i % 8;
            bitmap[byte_idx] |= (1 << bit_idx);
        }
    }
    if (bitmap_size > 0) {
        std::memcpy(out_buffer + offset, bitmap.data(), bitmap_size);
        offset += bitmap_size;
    }

    // 3. Escribir Directorio de Longitud Fija
    int fixed_dir_offset = offset;
    int fixed_entry_size = 1 + sizeof(int) + sizeof(int);
    offset += field_count * fixed_entry_size;

    int var_data_offset = offset; // Aqui empiezan los strings

    for (int i = 0; i < field_count; i++) {
        const auto& f = fields_[i];
        int current_entry = fixed_dir_offset + (i * fixed_entry_size);

        char tag = 'N'; // Null por defecto
        int val_or_offset = 0;
        int length = 0;

        if (!f.is_null) {
            if (f.type == FieldType::INT) {
                tag = 'I';
                val_or_offset = f.int_value;
            } else if (f.type == FieldType::STRING) {
                tag = 'S';
                val_or_offset = var_data_offset; // offset relativo al inicio del buffer
                length = static_cast<int>(f.str_value.size());
                
                // Escribir los datos variables de una vez
                std::memcpy(out_buffer + var_data_offset, f.str_value.data(), length);
                var_data_offset += length;
            }
        }

        out_buffer[current_entry] = tag;
        std::memcpy(out_buffer + current_entry + 1, &val_or_offset, sizeof(int));
        std::memcpy(out_buffer + current_entry + 1 + sizeof(int), &length, sizeof(int));
    }

    return var_data_offset;
}

Record Record::Deserialize(const char* buffer, int /*size*/) {
    Record record;
    int offset = 0;

    // 1. Leer field_count
    int field_count = 0;
    std::memcpy(&field_count, buffer + offset, sizeof(int));
    offset += sizeof(int);

    if (field_count == 0) return record;

    // 2. Leer Null Bitmap
    int bitmap_size = (field_count + 7) / 8;
    std::vector<char> bitmap(bitmap_size);
    std::memcpy(bitmap.data(), buffer + offset, bitmap_size);
    offset += bitmap_size;

    // 3. Leer Directorio Fijo e instanciar los fields
    int fixed_entry_size = 1 + sizeof(int) + sizeof(int);
    for (int i = 0; i < field_count; i++) {
        int byte_idx = i / 8;
        int bit_idx = i % 8;
        bool is_null = (bitmap[byte_idx] & (1 << bit_idx)) != 0;

        char tag = buffer[offset];
        int val_or_offset = 0;
        int length = 0;
        std::memcpy(&val_or_offset, buffer + offset + 1, sizeof(int));
        std::memcpy(&length, buffer + offset + 1 + sizeof(int), sizeof(int));
        offset += fixed_entry_size;

        if (is_null) {
            record.AddNull();
        } else {
            if (tag == 'I') {
                record.AddInt(val_or_offset);
            } else if (tag == 'S') {
                std::string val(buffer + val_or_offset, length);
                record.AddString(val);
            } else {
                record.AddNull(); // Fallback
            }
        }
    }

    return record;
}

int Record::FieldCount() const { return static_cast<int>(fields_.size()); }
bool Record::IsNull(int index) const { return fields_[index].is_null; }

int Record::GetInt(int index) const {
    if (fields_[index].is_null || fields_[index].type != FieldType::INT) return 0;
    return fields_[index].int_value;
}

std::string Record::GetString(int index) const {
    if (fields_[index].is_null || fields_[index].type != FieldType::STRING) return "";
    return fields_[index].str_value;
}

std::string Record::ToString() const {
    std::ostringstream oss;
    oss << "(";
    for (size_t i = 0; i < fields_.size(); i++) {
        if (fields_[i].is_null) {
            oss << "NULL";
        } else if (fields_[i].type == FieldType::INT) {
            oss << fields_[i].int_value;
        } else {
            oss << fields_[i].str_value;
        }
        if (i + 1 < fields_.size()) oss << ", ";
    }
    oss << ")";
    return oss.str();
}
