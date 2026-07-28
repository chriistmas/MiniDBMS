#include "heap_file.h"
#include "page.h"
#include <iostream>
#include <cstring>

HeapFile::HeapFile(IPageStore& page_store, int first_page_id)
    : page_store_(page_store), first_page_id_(first_page_id) {
    page_ids_.push_back(first_page_id_);
}

RID HeapFile::InsertRecord(const Record& record) {
    char buffer[PAGE_SIZE];
    int record_size = record.Serialize(buffer);

    // 1. Intentar insertar en alguna pagina existente que tenga espacio
    for (int page_id : page_ids_) {
        char page_data[PAGE_SIZE];
        page_store_.ReadPage(page_id, page_data);

        Page page(page_id);
        std::memcpy(page.GetData(), page_data, PAGE_SIZE);

        int slot_id = page.InsertRecord(buffer, record_size);
        if (slot_id != -1) {
            page_store_.WritePage(page_id, page.GetData());
            RID rid{page_id, slot_id};
            return rid;
        }
    }

    // 2. Ninguna pagina existente tiene espacio: solicitar una nueva
    int new_page_id = page_store_.AllocatePage();
    page_ids_.push_back(new_page_id);

    Page page(new_page_id);
    int slot_id = page.InsertRecord(buffer, record_size);
    page_store_.WritePage(new_page_id, page.GetData());

    RID rid{new_page_id, slot_id};
    return rid;
}

bool HeapFile::GetRecord(const RID& rid, Record& out_record) const {
    char page_data[PAGE_SIZE];
    page_store_.ReadPage(rid.page_id, page_data);

    Page page(rid.page_id);
    std::memcpy(page.GetData(), page_data, PAGE_SIZE);

    char record_buffer[PAGE_SIZE];
    int record_size = 0;
    if (!page.GetRecord(rid.slot_id, record_buffer, record_size)) {
        return false;
    }

    out_record = Record::Deserialize(record_buffer, record_size);
    return true;
}

bool HeapFile::DeleteRecord(const RID& rid) {
    char page_data[PAGE_SIZE];
    page_store_.ReadPage(rid.page_id, page_data);

    Page page(rid.page_id);
    std::memcpy(page.GetData(), page_data, PAGE_SIZE);

    if (!page.DeleteRecord(rid.slot_id)) {
        return false;
    }

    page_store_.WritePage(rid.page_id, page.GetData());
    return true;
}

bool HeapFile::UpdateRecord(const RID& rid, const Record& record) {
    char page_data[PAGE_SIZE];
    page_store_.ReadPage(rid.page_id, page_data);

    Page page(rid.page_id);
    std::memcpy(page.GetData(), page_data, PAGE_SIZE);

    char buffer[PAGE_SIZE];
    int record_size = record.Serialize(buffer);

    if (!page.UpdateRecord(rid.slot_id, buffer, record_size)) {
        return false;
    }

    page_store_.WritePage(rid.page_id, page.GetData());
    return true;
}

void HeapFile::ScanAll() const {
    for (int page_id : page_ids_) {
        char page_data[PAGE_SIZE];
        page_store_.ReadPage(page_id, page_data);

        Page page(page_id);
        std::memcpy(page.GetData(), page_data, PAGE_SIZE);

        int slot_count = page.GetSlotCount();
        for (int slot_id = 0; slot_id < slot_count; slot_id++) {
            char record_buffer[PAGE_SIZE];
            int record_size = 0;
            if (page.GetRecord(slot_id, record_buffer, record_size)) {
                Record r = Record::Deserialize(record_buffer, record_size);
                std::cout << "  RID(" << page_id << "," << slot_id << ") -> "
                          << r.ToString() << "\n";
            }
        }
    }
}

int HeapFile::GetFirstPageId() const { return first_page_id_; }
const std::vector<int>& HeapFile::GetPageIds() const { return page_ids_; }

// ---------------------------------------------------------------------
// HeapFile::Iterator
// ---------------------------------------------------------------------

HeapFile::Iterator::Iterator(IPageStore& page_store, const std::vector<int>& page_ids)
    : page_store_(page_store), page_ids_(page_ids) {}

void HeapFile::Iterator::Open() {
    page_index_ = 0;
    slot_index_ = 0;
}

bool HeapFile::Iterator::Next(RID& out_rid, Record& out_record) {
    while (page_index_ < page_ids_.size()) {
        int page_id = page_ids_[page_index_];

        char page_data[PAGE_SIZE];
        page_store_.ReadPage(page_id, page_data);

        Page page(page_id);
        std::memcpy(page.GetData(), page_data, PAGE_SIZE);

        int slot_count = page.GetSlotCount();
        while (slot_index_ < slot_count) {
            int slot = slot_index_++;
            char record_buffer[PAGE_SIZE];
            int record_size = 0;
            if (page.GetRecord(slot, record_buffer, record_size)) {
                out_rid = RID{page_id, slot};
                out_record = Record::Deserialize(record_buffer, record_size);
                return true;
            }
            // slot eliminado (tombstone): seguir buscando
        }

        // Se agoto esta pagina, pasar a la siguiente
        page_index_++;
        slot_index_ = 0;
    }
    return false;
}

void HeapFile::Iterator::Close() {
    // No hay recursos que liberar (page_store_ es externo); se deja el
    // metodo por simetria con el resto de operadores del motor (Open/
    // Next/Close), y como punto de extension futuro (p.ej. liberar pines
    // si en el futuro el iterador mantuviera una pagina fetch-eada).
}
