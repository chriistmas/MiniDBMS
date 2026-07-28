#include <iostream>
#include <vector>
#include "disk_manager.h"
#include "buffer_pool_manager.h"
#include "buffer_pool_adapter.h"
#include "heap_file.h"
#include "hash_index.h"
#include "bplus_tree.h"

static void Section(const std::string& title) {
    std::cout << "\n########################################################\n";
    std::cout << "# " << title << "\n";
    std::cout << "########################################################\n";
}

int main() {
    Section("1. Preparar disco + Buffer Pool + una tabla de ejemplo (HeapFile)");
    DiskManager disk_manager("index_test.db");
    disk_manager.CreateDatabase();

    BufferPoolManager bpm(disk_manager, /*pool_size=*/16);
    BufferPoolAdapter adapter(bpm); // <-- todo el modulo de indices trabaja sobre esto

    int table_first_page = adapter.AllocatePage();
    HeapFile table(adapter, table_first_page);

    std::vector<int> keys = {50, 20, 80, 10, 30, 60, 90, 5, 25, 40, 70, 100, 15, 55, 85};
    std::vector<RID> rids;
    for (int k : keys) {
        Record r;
        r.AddInt(k);
        r.AddString("valor_" + std::to_string(k));
        rids.push_back(table.InsertRecord(r));
    }
    std::cout << "Insertados " << keys.size() << " registros en la tabla de ejemplo.\n";

    Section("2. Arbol B+ (orden pequeno = 4) construido SOBRE el Buffer Pool");
    BPlusTree bptree(adapter, /*max_entries=*/4);
    for (size_t i = 0; i < keys.size(); i++) {
        bptree.Insert(keys[i], rids[i]);
    }
    std::cout << "Raiz del arbol: pagina " << bptree.GetRootPageId()
              << " (crecio por encima de una sola hoja: hubo splits)\n";
    bptree.PrintLeaves();

    std::cout << "\nBusquedas puntuales:\n";
    for (int q : {30, 999, 100, 5}) {
        std::vector<RID> found;
        bool ok = bptree.Search(q, found);
        std::cout << "  Search(" << q << ") -> " << (ok ? "ENCONTRADO" : "no existe");
        if (ok) {
            for (auto& rid : found) std::cout << " RID(" << rid.page_id << "," << rid.slot_id << ")";
        }
        std::cout << "\n";
    }

    Section("3. Indice Hash Estatico (existente) reutilizado sobre el Buffer Pool");
    StaticHashIndex hash_index(adapter, /*num_buckets=*/4);
    for (size_t i = 0; i < keys.size(); i++) {
        hash_index.Insert(keys[i], rids[i]);
    }
    std::vector<RID> hash_result;
    bool hash_ok = hash_index.Search(60, hash_result);
    std::cout << "StaticHashIndex.Search(60) -> " << (hash_ok ? "ENCONTRADO" : "no existe");
    for (auto& rid : hash_result) std::cout << " RID(" << rid.page_id << "," << rid.slot_id << ")";
    std::cout << "\n";
    std::cout << "(Igual que en el modulo de Disco: colisiones resueltas via overflow chaining,\n"
              << " solo que ahora cada Insert/Search pasa por el Buffer Pool en vez de ir\n"
              << " directo a disco.)\n";

    Section("4. Cross-check: comparar resultados del indice contra la tabla real");
    for (int q : {30, 100}) {
        std::vector<RID> found;
        bptree.Search(q, found);
        for (auto& rid : found) {
            Record rec;
            table.GetRecord(rid, rec);
            std::cout << "  Clave " << q << " -> " << rec.ToString() << " (via B+Tree)\n";
        }
    }

    bpm.FlushAllPages();
    std::cout << "\nHits: " << bpm.GetHitCount() << "  Misses: " << bpm.GetMissCount() << "\n";
    disk_manager.CloseDatabase();
    std::cout << "\nPrueba del modulo de Indexacion finalizada correctamente.\n";
    return 0;
}
