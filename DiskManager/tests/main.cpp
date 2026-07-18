#include <iostream>
#include <cstring>
#include "disk_manager.h"
#include "page.h"
#include "record.h"
#include "heap_file.h"
#include "catalog.h"
#include "csv_loader.h"

void PrintSectionTitle(const std::string& title) {
    std::cout << "\n";
    std::cout << "########################################################\n";
    std::cout << "# " << title << "\n";
    std::cout << "########################################################\n";
}

int main() {
    const std::string DB_NAME = "universidad.db";

    // ---------------------------------------------------------------
    // 1. CREACION DEL DISCO (archivo fisico + simulacion de superficies)
    // ---------------------------------------------------------------
    PrintSectionTitle("1. Creacion del disco / base de datos");

    DiskManager disk_manager(DB_NAME);
    if (!disk_manager.CreateDatabase()) {
        std::cerr << "Fallo al crear la base de datos.\n";
        return 1;
    }

    std::cout << "Paginas actuales en el disco: " << disk_manager.GetNumPages() << "\n";

    // ---------------------------------------------------------------
    // 2. CATALOGO + CARGA DESDE CSV
    // ---------------------------------------------------------------
    PrintSectionTitle("2. Carga de datos desde CSV");

    Catalog catalog;
    catalog.SetDatabaseName("universidad");

    HeapFile* alumnos_heap = nullptr;
    bool loaded = CsvLoader::LoadCsvIntoTable(
        "data/alumnos.csv", "alumnos", disk_manager, catalog, &alumnos_heap);

    if (!loaded || alumnos_heap == nullptr) {
        std::cerr << "Fallo al cargar el CSV.\n";
        return 1;
    }

    catalog.Print();

    std::cout << "\nRegistros insertados en la tabla 'alumnos':\n";
    alumnos_heap->ScanAll();

    // ---------------------------------------------------------------
    // 3. PRUEBAS MANUALES DE PAGE / SLOTTED PAGE (insert, get, update, delete)
    // ---------------------------------------------------------------
    PrintSectionTitle("3. Pruebas manuales sobre una pagina (slotted page)");

    int cursos_page_id = disk_manager.AllocatePage();
    std::cout << "Nueva pagina asignada para 'cursos' -> Page ID: " << cursos_page_id << "\n";

    Page page(cursos_page_id);

    Record curso1;
    curso1.AddInt(101);
    curso1.AddString("BaseDeDatos");
    char buf1[PAGE_SIZE];
    int size1 = curso1.Serialize(buf1);
    int slot1 = page.InsertRecord(buf1, size1);

    Record curso2;
    curso2.AddInt(102);
    curso2.AddString("SistemasOperativos");
    char buf2[PAGE_SIZE];
    int size2 = curso2.Serialize(buf2);
    int slot2 = page.InsertRecord(buf2, size2);

    std::cout << "Insertado curso1 en RID(" << cursos_page_id << "," << slot1 << ") -> "
              << curso1.ToString() << "\n";
    std::cout << "Insertado curso2 en RID(" << cursos_page_id << "," << slot2 << ") -> "
              << curso2.ToString() << "\n";
    std::cout << "Registros en la pagina: " << page.GetRecordCount()
              << " | Espacio libre: " << page.GetFreeSpace() << " bytes\n";

    disk_manager.WritePage(cursos_page_id, page.GetData());

    // Leer de vuelta desde disco (simulando una relectura)
    char reread_data[PAGE_SIZE];
    disk_manager.ReadPage(cursos_page_id, reread_data);
    Page page_reread(cursos_page_id);
    std::memcpy(page_reread.GetData(), reread_data, PAGE_SIZE);

    char out_buf[PAGE_SIZE];
    int out_size = 0;
    page_reread.GetRecord(slot1, out_buf, out_size);
    Record leido = Record::Deserialize(out_buf, out_size);
    std::cout << "Releido desde disco RID(" << cursos_page_id << "," << slot1 << ") -> "
              << leido.ToString() << "\n";

    // Actualizar
    Record curso1_actualizado;
    curso1_actualizado.AddInt(101);
    curso1_actualizado.AddString("BaseDeDatosAvanzada");
    char buf_upd[PAGE_SIZE];
    int size_upd = curso1_actualizado.Serialize(buf_upd);
    page_reread.UpdateRecord(slot1, buf_upd, size_upd);
    disk_manager.WritePage(cursos_page_id, page_reread.GetData());
    std::cout << "RID(" << cursos_page_id << "," << slot1 << ") actualizado a -> "
              << curso1_actualizado.ToString() << "\n";

    // Eliminar
    page_reread.DeleteRecord(slot2);
    disk_manager.WritePage(cursos_page_id, page_reread.GetData());
    std::cout << "RID(" << cursos_page_id << "," << slot2 << ") eliminado.\n";
    std::cout << "Registros activos restantes en la pagina: "
              << page_reread.GetRecordCount() << "\n";

    // ---------------------------------------------------------------
    // 4. PRUEBA DE PERSISTENCIA: cerrar y volver a abrir el archivo
    // ---------------------------------------------------------------
    PrintSectionTitle("4. Prueba de persistencia (cerrar y reabrir el disco)");

    disk_manager.CloseDatabase();
    std::cout << "Disco cerrado.\n";

    DiskManager disk_manager_2(DB_NAME);
    if (!disk_manager_2.OpenDatabase()) {
        std::cerr << "Error al reabrir la base de datos.\n";
        return 1;
    }
    std::cout << "Disco reabierto. Paginas encontradas: "
              << disk_manager_2.GetNumPages() << "\n";

    HeapFile alumnos_heap_2(disk_manager_2, alumnos_heap->GetFirstPageId());
    std::cout << "Contenido de 'alumnos' tras reabrir el disco:\n";
    alumnos_heap_2.ScanAll();

    delete alumnos_heap;

    std::cout << "\nPrueba completa. El archivo '" << DB_NAME
              << "' quedo en el directorio actual.\n";

    return 0;
}
