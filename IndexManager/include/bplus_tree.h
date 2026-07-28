#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include <vector>
#include "i_page_store.h"
#include "heap_file.h" // for RID

/*
 * BPlusTree
 * -------------------------------------------------------------------------
 * Indice Arbol B+ mapeado sobre paginas fisicas (IPageStore), es decir,
 * cada NODO del arbol (hoja o interno) vive dentro de UNA pagina del
 * gestor de paginas (DiskManager directo, o BufferPoolManager mediante
 * BufferPoolAdapter).
 *
 * Representacion de un nodo dentro de su Page (reutiliza el mecanismo de
 * Slotted Page ya existente, sin modificar Page/PageHeader):
 *
 *   Slot 0            -> Record de 1 entero: 1 = hoja, 0 = interno (meta).
 *   Slots 1..k (hoja)     -> Record(key, rid.page_id, rid.slot_id).
 *   Slots 1..k (interno)  -> Record(key, right_child_page_id).
 *
 *   PageHeader.next_page_id se reutiliza con dos significados segun el
 *   tipo de nodo:
 *     - Hoja:     puntero a la SIGUIENTE hoja (permite scans ordenados).
 *     - Interno:  puntero al hijo MAS IZQUIERDO (antes de la 1ra key).
 *
 * Insercion: descenso recursivo con pila de paginas visitadas (no hay
 * punteros a padre). Si un nodo se llena (mas de max_entries_ elementos)
 * se divide en dos y la clave separadora se propaga hacia el padre; si
 * el nodo que se divide es la raiz, se crea una nueva raiz.
 *
 * Solo soporta claves de busqueda de tipo INTEGER (igual que StaticHashIndex).
 * -------------------------------------------------------------------------
 */
class BPlusTree {
public:
    // max_entries: maximo de (clave, referencia) por nodo antes de dividir.
    // Un valor pequeno (por defecto 4) facilita observar splits en las
    // pruebas/evidencias con pocos datos; en un caso real se calcularia
    // en funcion de PAGE_SIZE para maximizar el fanout.
    explicit BPlusTree(IPageStore& page_store, int max_entries = 4, int root_page_id = -1);

    int GetRootPageId() const;

    // Inserta clave -> RID. Permite claves repetidas (todas se conservan).
    void Insert(int key, const RID& rid);

    // Busca todas las ocurrencias de una clave.
    bool Search(int key, std::vector<RID>& out_rids) const;

    // Recorre todas las hojas en orden (usando el encadenamiento de hojas)
    // e imprime su contenido; util para depuracion/evidencias.
    void PrintLeaves() const;

private:
    IPageStore& page_store_;
    int max_entries_;
    int root_page_id_;

    struct Entry {
        int key;
        int a; // rid.page_id (hoja) o right_child_page_id (interno)
        int b; // rid.slot_id (hoja) o -1 (interno)
    };

    bool IsLeaf(int page_id) const;
    std::vector<Entry> ReadEntries(int page_id) const;
    int ReadLeftmostOrNext(int page_id) const; // next_page_id crudo de la pagina

    void WriteLeaf(int page_id, const std::vector<Entry>& entries, int next_leaf_id);
    void WriteInternal(int page_id, const std::vector<Entry>& entries, int leftmost_child);

    int CreateEmptyLeaf();
    int CreateEmptyInternal(int leftmost_child);

    // Resultado de una insercion recursiva: si el nodo hijo se dividio,
    // se informa la clave promovida y el page_id del nuevo nodo derecho.
    struct SplitResult {
        bool split = false;
        int promoted_key = 0;
        int new_right_page_id = -1;
    };

    SplitResult InsertRecursive(int page_id, int key, const RID& rid);
};

#endif // BPLUS_TREE_H
