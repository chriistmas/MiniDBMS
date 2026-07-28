#ifndef HEAP_FILE_H
#define HEAP_FILE_H

#include <vector>
#include "i_page_store.h"
#include "record.h"

/*
 * RID (Record ID)
 * -------------------------------------------------------------------------
 * RID = (Page ID, Slot ID)
 * Identifica un registro sin apuntar directamente a su direccion fisica.
 * Esto permite mover el registro dentro de la pagina (por ejemplo al
 * compactar espacio) sin invalidar referencias externas: solo cambia el
 * contenido del slot, el RID sigue siendo valido.
 * -------------------------------------------------------------------------
 */
struct RID {
    int page_id;
    int slot_id;
};

/*
 * HeapFile
 * -------------------------------------------------------------------------
 * Coleccion de paginas donde los registros de una tabla se almacenan sin
 * un orden especifico. Usa un IPageStore (DiskManager directo, o un
 * BufferPoolManager a traves de su adaptador) para leer/escribir las
 * paginas fisicas que le pertenecen.
 * -------------------------------------------------------------------------
 */
class HeapFile {
public:
    HeapFile(IPageStore& page_store, int first_page_id);

    // Inserta un registro. Si ninguna pagina existente tiene espacio,
    // se solicita una nueva pagina al DiskManager.
    RID InsertRecord(const Record& record);

    bool GetRecord(const RID& rid, Record& out_record) const;
    bool DeleteRecord(const RID& rid);
    bool UpdateRecord(const RID& rid, const Record& record);

    // Recorre todas las paginas e imprime los registros activos (para pruebas).
    void ScanAll() const;

    int GetFirstPageId() const;
    const std::vector<int>& GetPageIds() const;

private:
    IPageStore& page_store_;
    int first_page_id_;
    std::vector<int> page_ids_; // paginas que pertenecen a esta tabla
};

#endif // HEAP_FILE_H
