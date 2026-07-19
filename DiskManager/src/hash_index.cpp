#include "hash_index.h"
#include "page.h"
#include "record.h"
#include <cstring>
#include <iostream>

StaticHashIndex::StaticHashIndex(DiskManager& disk_manager, int num_buckets, int first_page_id)
    : disk_manager_(disk_manager), num_buckets_(num_buckets), first_page_id_(first_page_id) {
    
    if (first_page_id_ == -1) {
        // Reservar paginas contiguas para las cubetas principales
        for (int i = 0; i < num_buckets_; i++) {
            int pid = disk_manager_.AllocatePage();
            if (i == 0) first_page_id_ = pid;
        }
    }
}

int StaticHashIndex::GetFirstPageId() const { return first_page_id_; }
int StaticHashIndex::GetNumBuckets() const { return num_buckets_; }

int StaticHashIndex::Hash(int key) const {
    // Funcion Hash simple (como recomienda hash.txt, aunque para enteros es trivial)
    unsigned int hash = static_cast<unsigned int>(key);
    return hash % num_buckets_;
}

bool StaticHashIndex::Insert(int key, const RID& rid) {
    int bucket_idx = Hash(key);
    int current_page_id = first_page_id_ + bucket_idx;

    Record index_entry;
    index_entry.AddInt(key);
    index_entry.AddInt(rid.page_id);
    index_entry.AddInt(rid.slot_id);

    char record_buffer[PAGE_SIZE];
    int record_size = index_entry.Serialize(record_buffer);

    while (true) {
        char page_data[PAGE_SIZE];
        disk_manager_.ReadPage(current_page_id, page_data);
        
        Page page(current_page_id);
        std::memcpy(page.GetData(), page_data, PAGE_SIZE);

        int slot = page.InsertRecord(record_buffer, record_size);
        if (slot != -1) {
            // Se inserto exitosamente
            disk_manager_.WritePage(current_page_id, page.GetData());
            return true;
        }

        // Si no cupo, ver si hay pagina de desbordamiento (overflow)
        int next_page_id = page.GetNextPageId();
        if (next_page_id == -1) {
            // Crear una nueva pagina de desbordamiento (overflow chaining)
            next_page_id = disk_manager_.AllocatePage();
            page.SetNextPageId(next_page_id);
            disk_manager_.WritePage(current_page_id, page.GetData()); // actualizar puntero
        }
        current_page_id = next_page_id;
    }
    return false;
}

bool StaticHashIndex::Search(int key, std::vector<RID>& out_rids) const {
    int bucket_idx = Hash(key);
    int current_page_id = first_page_id_ + bucket_idx;
    bool found = false;

    while (current_page_id != -1) {
        char page_data[PAGE_SIZE];
        disk_manager_.ReadPage(current_page_id, page_data);
        
        Page page(current_page_id);
        std::memcpy(page.GetData(), page_data, PAGE_SIZE);

        int slot_count = page.GetSlotCount();
        for (int i = 0; i < slot_count; i++) {
            char rec_buf[PAGE_SIZE];
            int rec_size = 0;
            if (page.GetRecord(i, rec_buf, rec_size)) {
                Record r = Record::Deserialize(rec_buf, rec_size);
                // Validar que es una entrada de indice
                if (r.FieldCount() == 3 && !r.IsNull(0) && r.GetInt(0) == key) {
                    RID rid;
                    rid.page_id = r.GetInt(1);
                    rid.slot_id = r.GetInt(2);
                    out_rids.push_back(rid);
                    found = true;
                }
            }
        }
        current_page_id = page.GetNextPageId();
    }
    return found;
}
