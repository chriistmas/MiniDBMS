#ifndef I_PAGE_STORE_H
#define I_PAGE_STORE_H

/*
 * IPageStore
 * -------------------------------------------------------------------------
 * Contrato minimo que necesita cualquier capa que trabaje a nivel de
 * paginas (HeapFile, StaticHashIndex, BPlusTree, etc.):
 *
 *   - Leer una pagina completa hacia un buffer.
 *   - Escribir una pagina completa desde un buffer.
 *   - Pedir una pagina nueva / liberar una pagina.
 *
 * DiskManager implementa esta interfaz accediendo directamente al disco
 * (sin cache). BufferPoolManager (modulo BufferManager) tambien la
 * implementa mediante un adaptador que intercepta esas mismas llamadas
 * y las resuelve contra frames en RAM, aplicando una politica de
 * reemplazo antes de tocar el disco.
 *
 * Gracias a esto, HeapFile / StaticHashIndex / BPlusTree se escriben UNA
 * sola vez y funcionan igual "con" o "sin" Buffer Pool: solo cambia la
 * implementacion de IPageStore que reciben por referencia.
 * -------------------------------------------------------------------------
 */
class IPageStore {
public:
    virtual ~IPageStore() = default;

    virtual void ReadPage(int page_id, char* page_data) = 0;
    virtual void WritePage(int page_id, const char* page_data) = 0;

    virtual int AllocatePage() = 0;
    virtual void DeallocatePage(int page_id) = 0;

    virtual int GetNumPages() const = 0;
};

#endif // I_PAGE_STORE_H
