#include "page.h"
#include <cstring>

Page::Page() {
    InitEmptyPage(-1);
}

Page::Page(int page_id) {
    InitEmptyPage(page_id);
}

void Page::InitEmptyPage(int page_id) {
    std::memset(data_, 0, PAGE_SIZE);
    PageHeader* h = Header();
    h->page_id = page_id;
    h->record_count = 0;
    h->slot_count = 0;
    h->free_space_pointer = PAGE_SIZE;
    h->free_space = PAGE_SIZE - static_cast<int>(sizeof(PageHeader));
}

char* Page::GetData() { return data_; }
const char* Page::GetData() const { return data_; }

int Page::GetPageId() const { return Header()->page_id; }
void Page::SetPageId(int page_id) { Header()->page_id = page_id; }

int Page::GetRecordCount() const { return Header()->record_count; }
int Page::GetSlotCount() const { return Header()->slot_count; }
int Page::GetFreeSpace() const { return Header()->free_space; }

PageHeader* Page::Header() { return reinterpret_cast<PageHeader*>(data_); }
const PageHeader* Page::Header() const { return reinterpret_cast<const PageHeader*>(data_); }

Slot* Page::SlotArray() { return reinterpret_cast<Slot*>(data_ + sizeof(PageHeader)); }
const Slot* Page::SlotArray() const { return reinterpret_cast<const Slot*>(data_ + sizeof(PageHeader)); }

int Page::InsertRecord(const char* record_data, int record_size) {
    PageHeader* h = Header();

    // Buscar un slot eliminado (tombstone) para reutilizar
    Slot* slots = SlotArray();
    int reuse_slot = -1;
    for (int i = 0; i < h->slot_count; i++) {
        if (slots[i].offset == -1) {
            reuse_slot = i;
            break;
        }
    }

    int extra_slot_cost = (reuse_slot == -1) ? static_cast<int>(sizeof(Slot)) : 0;
    int needed = record_size + extra_slot_cost;
    if (needed > h->free_space) {
        return -1; // no cabe en esta pagina
    }

    int new_offset = h->free_space_pointer - record_size;
    std::memcpy(data_ + new_offset, record_data, record_size);

    int slot_id = reuse_slot;
    if (slot_id == -1) {
        slot_id = h->slot_count;
        h->slot_count++;
        h->free_space -= static_cast<int>(sizeof(Slot));
    }

    slots[slot_id].offset = new_offset;
    slots[slot_id].length = record_size;

    h->free_space_pointer = new_offset;
    h->free_space -= record_size;
    h->record_count++;

    return slot_id;
}

bool Page::GetRecord(int slot_id, char* out_buffer, int& out_size) const {
    const PageHeader* h = Header();
    if (slot_id < 0 || slot_id >= h->slot_count) return false;

    const Slot* slots = SlotArray();
    if (slots[slot_id].offset == -1) return false;

    out_size = slots[slot_id].length;
    std::memcpy(out_buffer, data_ + slots[slot_id].offset, out_size);
    return true;
}

bool Page::DeleteRecord(int slot_id) {
    PageHeader* h = Header();
    if (slot_id < 0 || slot_id >= h->slot_count) return false;

    Slot* slots = SlotArray();
    if (slots[slot_id].offset == -1) return false;

    h->free_space += slots[slot_id].length;
    h->record_count--;
    slots[slot_id].offset = -1;
    slots[slot_id].length = -1;
    return true;
}

bool Page::UpdateRecord(int slot_id, const char* record_data, int record_size) {
    PageHeader* h = Header();
    if (slot_id < 0 || slot_id >= h->slot_count) return false;

    Slot* slots = SlotArray();
    if (slots[slot_id].offset == -1) return false;

    // Caso 1: el nuevo registro cabe en el mismo espacio -> sobrescribir in-place
    if (record_size <= slots[slot_id].length) {
        std::memcpy(data_ + slots[slot_id].offset, record_data, record_size);
        h->free_space += (slots[slot_id].length - record_size);
        slots[slot_id].length = record_size;
        return true;
    }

    // Caso 2: no cabe -> liberar espacio actual y reinsertar en una nueva posicion
    int old_len = slots[slot_id].length;
    h->free_space += old_len;
    slots[slot_id].offset = -1;
    slots[slot_id].length = -1;
    h->record_count--;

    if (record_size > h->free_space) {
        return false; // no hay espacio suficiente en esta pagina
    }

    int new_offset = h->free_space_pointer - record_size;
    std::memcpy(data_ + new_offset, record_data, record_size);

    slots[slot_id].offset = new_offset;
    slots[slot_id].length = record_size;
    h->free_space_pointer = new_offset;
    h->free_space -= record_size;
    h->record_count++;

    return true;
}
