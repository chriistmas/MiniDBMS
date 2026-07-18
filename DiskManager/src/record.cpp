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

int Record::SerializedSize() const {
    int size = static_cast<int>(sizeof(int)); // field_count
    for (const auto& f : fields_) {
        size += 1; // tag
        if (f.type == FieldType::INT) {
            size += static_cast<int>(sizeof(int));
        } else {
            size += static_cast<int>(sizeof(int)) + static_cast<int>(f.str_value.size());
        }
    }
    return size;
}

int Record::Serialize(char* out_buffer) const {
    int offset = 0;
    int field_count = static_cast<int>(fields_.size());
    std::memcpy(out_buffer + offset, &field_count, sizeof(int));
    offset += sizeof(int);

    for (const auto& f : fields_) {
        char tag = (f.type == FieldType::INT) ? 'I' : 'S';
        out_buffer[offset] = tag;
        offset += 1;

        if (f.type == FieldType::INT) {
            std::memcpy(out_buffer + offset, &f.int_value, sizeof(int));
            offset += sizeof(int);
        } else {
            int len = static_cast<int>(f.str_value.size());
            std::memcpy(out_buffer + offset, &len, sizeof(int));
            offset += sizeof(int);
            std::memcpy(out_buffer + offset, f.str_value.data(), len);
            offset += len;
        }
    }
    return offset;
}

Record Record::Deserialize(const char* buffer, int /*size*/) {
    Record record;
    int offset = 0;

    int field_count = 0;
    std::memcpy(&field_count, buffer + offset, sizeof(int));
    offset += sizeof(int);

    for (int i = 0; i < field_count; i++) {
        char tag = buffer[offset];
        offset += 1;

        if (tag == 'I') {
            int value = 0;
            std::memcpy(&value, buffer + offset, sizeof(int));
            offset += sizeof(int);
            record.AddInt(value);
        } else {
            int len = 0;
            std::memcpy(&len, buffer + offset, sizeof(int));
            offset += sizeof(int);
            std::string value(buffer + offset, len);
            offset += len;
            record.AddString(value);
        }
    }
    return record;
}

int Record::FieldCount() const { return static_cast<int>(fields_.size()); }

int Record::GetInt(int index) const {
    return fields_[index].int_value;
}

std::string Record::GetString(int index) const {
    return fields_[index].str_value;
}

std::string Record::ToString() const {
    std::ostringstream oss;
    oss << "(";
    for (size_t i = 0; i < fields_.size(); i++) {
        if (fields_[i].type == FieldType::INT) {
            oss << fields_[i].int_value;
        } else {
            oss << fields_[i].str_value;
        }
        if (i + 1 < fields_.size()) oss << ", ";
    }
    oss << ")";
    return oss.str();
}
