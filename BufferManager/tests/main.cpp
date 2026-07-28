#include <iostream>
#include <cassert>
#include "disk_manager.h"
#include "buffer_pool_manager.h"
#include "buffer_pool_adapter.h"
#include "heap_file.h"

static void Section(const std::string& title) {
    std::cout << "\n########################################################\n";
    std::cout << "# " << title << "\n";
    std::cout << "########################################################\n";
}

int main() {
    Section("1. Creacion del disco de pruebas");
    DiskManager disk_manager("buffer_pool_test.db");
    disk_manager.CreateDatabase();

    Section("2. Buffer Pool pequeno (3 frames) para forzar reemplazo LRU");
    BufferPoolManager bpm(disk_manager, /*pool_size=*/3);

    int p1, p2, p3, p4;
    Page* page1 = bpm.NewPage(&p1);
    (void)page1;
    bpm.UnpinPage(p1, true);

    Page* page2 = bpm.NewPage(&p2);
    (void)page2;
    bpm.UnpinPage(p2, true);

    Page* page3 = bpm.NewPage(&p3);
    (void)page3;
    bpm.UnpinPage(p3, true);

    std::cout << "Paginas creadas: " << p1 << ", " << p2 << ", " << p3 << "\n";
    std::cout << "Frames usados (pool_size=3): todos los frames estan ocupados.\n";

    // Traer p1 de nuevo la vuelve "recientemente usada" (no debe ser victima)
    bpm.FetchPage(p1);
    bpm.UnpinPage(p1, false);

    // Pedir una 4ta pagina: el Buffer Pool debe elegir victima via LRU
    // (deberia desalojar p2, que es la menos recientemente usada tras el paso anterior)
    Page* page4 = bpm.NewPage(&p4);
    (void)page4;
    bpm.UnpinPage(p4, true);
    std::cout << "Pagina 4 creada (" << p4 << ") forzando desalojo LRU de un frame.\n";

    std::cout << "Hits: " << bpm.GetHitCount() << "  Misses: " << bpm.GetMissCount() << "\n";

    // p1 sigue siendo recuperable correctamente aunque haya habido reemplazo
    Page* p1_again = bpm.FetchPage(p1);
    assert(p1_again->GetPageId() == p1);
    bpm.UnpinPage(p1, false);
    std::cout << "Verificado: Pagina " << p1 << " se recupera correctamente tras el reemplazo.\n";

    Section("3. HeapFile funcionando A TRAVES del Buffer Pool (no directo a disco)");
    BufferPoolAdapter adapter(bpm);

    int heap_first_page = disk_manager.AllocatePage();
    HeapFile heap(adapter, heap_first_page);

    Record r1; r1.AddInt(1); r1.AddString("Ana");
    Record r2; r2.AddInt(2); r2.AddString("Luis");
    RID rid1 = heap.InsertRecord(r1);
    RID rid2 = heap.InsertRecord(r2);

    std::cout << "Insertados via BufferPool -> RID(" << rid1.page_id << "," << rid1.slot_id
              << ") y RID(" << rid2.page_id << "," << rid2.slot_id << ")\n";

    bpm.FlushAllPages();
    std::cout << "FlushAllPages(): paginas sucias sincronizadas a disco.\n";

    Record out;
    bool ok = heap.GetRecord(rid1, out);
    std::cout << "Lectura de vuelta -> " << (ok ? out.ToString() : "ERROR") << "\n";

    Section("4. Estadisticas finales del Buffer Pool");
    std::cout << "Pool size: " << bpm.GetPoolSize() << "\n";
    std::cout << "Hits: " << bpm.GetHitCount() << "  Misses: " << bpm.GetMissCount() << "\n";
    std::cout << "Paginas totales en disco: " << bpm.GetNumPages() << "\n";

    disk_manager.CloseDatabase();
    std::cout << "\nPrueba de Buffer Pool Manager finalizada correctamente.\n";
    return 0;
}
