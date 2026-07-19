#ifndef HASH_INDEX_H
#define HASH_INDEX_H

#include "disk_manager.h"
#include "heap_file.h" // for RID
#include <vector>

/*
 * StaticHashIndex
 * -------------------------------------------------------------------------
 * Indice Hash Estatico. Utiliza un numero fijo de cubetas (buckets).
 * Si una cubeta se llena, usa "overflow chaining" (encadenamiento de 
 * desbordamiento) pidiendo nuevas paginas al DiskManager y enlazandolas 
 * mediante next_page_id de la pagina.
 * 
 * Cada entrada del indice es un Record(Key, PageID, SlotID).
 * Solo soporta claves de busqueda de tipo INTEGER.
 * -------------------------------------------------------------------------
 */
class StaticHashIndex {
public:
    // Crea un nuevo indice o abre uno existente.
    // Si first_page_id es -1, asigna num_buckets paginas consecutivas.
    StaticHashIndex(DiskManager& disk_manager, int num_buckets, int first_page_id = -1);

    int GetFirstPageId() const;
    int GetNumBuckets() const;

    // Inserta una entrada (clave -> RID) en el indice
    bool Insert(int key, const RID& rid);

    // Busca todas las ocurrencias de una clave y devuelve sus RIDs
    bool Search(int key, std::vector<RID>& out_rids) const;

private:
    DiskManager& disk_manager_;
    int num_buckets_;
    int first_page_id_;

    int Hash(int key) const;
};

#endif // HASH_INDEX_H
