#ifndef BUFFER_POOL_ADAPTER_H
#define BUFFER_POOL_ADAPTER_H

#include "i_page_store.h"
#include "buffer_pool_manager.h"

/*
 * BufferPoolAdapter
 * -------------------------------------------------------------------------
 * Adaptador (patron Adapter) que expone un BufferPoolManager con la misma
 * interfaz "de pagina cruda" (IPageStore) que ya usan HeapFile y
 * StaticHashIndex (ver DiskManager/include/i_page_store.h).
 *
 * Esto permite reutilizar HeapFile / StaticHashIndex / futuros indices SIN
 * modificarlos: en vez de:
 *
 *      HeapFile hf(disk_manager, first_page_id);       // sin cache
 *
 * ahora se puede escribir:
 *
 *      BufferPoolManager bpm(disk_manager, 64);
 *      BufferPoolAdapter adapter(bpm);
 *      HeapFile hf(adapter, first_page_id);             // con cache LRU
 *
 * Cada ReadPage/WritePage se traduce internamente a Fetch/Unpin sobre el
 * Buffer Pool, por lo que las paginas "calientes" ya no van a disco en
 * cada operacion.
 * -------------------------------------------------------------------------
 */
class BufferPoolAdapter : public IPageStore {
public:
    explicit BufferPoolAdapter(BufferPoolManager& bpm);

    void ReadPage(int page_id, char* page_data) override;
    void WritePage(int page_id, const char* page_data) override;

    int AllocatePage() override;
    void DeallocatePage(int page_id) override;

    int GetNumPages() const override;

    BufferPoolManager& GetBufferPoolManager() { return bpm_; }

private:
    BufferPoolManager& bpm_;
};

#endif // BUFFER_POOL_ADAPTER_H
