#include <iostream>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include "disk_manager.h"
#include "page.h"
#include "record.h"
#include "heap_file.h"
#include "catalog.h"
#include "csv_loader.h"
#include "hash_index.h"

namespace fs = std::filesystem;

void PrintSectionTitle(const std::string& title) {
    std::cout << "\n";
    std::cout << "########################################################\n";
    std::cout << "# " << title << "\n";
    std::cout << "########################################################\n";
}

// ---------------------------------------------------------------
// Lista los archivos .csv disponibles en un directorio
// ---------------------------------------------------------------
std::vector<std::string> ListCsvFiles(const std::string& directory) {
    std::vector<std::string> csv_files;
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        return csv_files;
    }
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            csv_files.push_back(entry.path().string());
        }
    }
    std::sort(csv_files.begin(), csv_files.end());
    return csv_files;
}

// ---------------------------------------------------------------
// Menu interactivo para elegir CSV
// ---------------------------------------------------------------
std::string ElegirCsv() {
    const std::string DATA_DIR = "data";
    std::vector<std::string> csv_files = ListCsvFiles(DATA_DIR);

    std::cout << "\n========================================\n";
    std::cout << "  Seleccione un archivo CSV para cargar\n";
    std::cout << "========================================\n\n";

    if (!csv_files.empty()) {
        std::cout << "Archivos CSV encontrados en '" << DATA_DIR << "/':\n\n";
        for (size_t i = 0; i < csv_files.size(); i++) {
            std::cout << "  [" << (i + 1) << "] " << csv_files[i] << "\n";
        }
        std::cout << "\n  [0] Escribir una ruta personalizada\n";
    } else {
        std::cout << "No se encontraron archivos CSV en '" << DATA_DIR << "/'.\n";
        std::cout << "Debe escribir la ruta al archivo CSV manualmente.\n";
    }

    std::cout << "\nOpcion: ";
    std::string input;
    std::getline(std::cin, input);

    // Quitar espacios sobrantes
    while (!input.empty() && input.back() == ' ') input.pop_back();
    while (!input.empty() && input.front() == ' ') input.erase(input.begin());

    if (input.empty()) {
        std::cerr << "Entrada vacia. Saliendo.\n";
        return "";
    }

    // Intentar interpretar como numero
    bool is_number = true;
    for (char c : input) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            is_number = false;
            break;
        }
    }

    if (is_number) {
        int option = std::stoi(input);
        if (option == 0) {
            std::cout << "Escriba la ruta completa al archivo CSV: ";
            std::string custom_path;
            std::getline(std::cin, custom_path);
            return custom_path;
        }
        if (option >= 1 && option <= static_cast<int>(csv_files.size())) {
            return csv_files[option - 1];
        }
        std::cerr << "Opcion invalida: " << option << "\n";
        return "";
    }

    // Si no es numero, asumir que es una ruta directa
    return input;
}

// ---------------------------------------------------------------
// Pide al usuario el nombre de la tabla
// ---------------------------------------------------------------
std::string PedirNombreTabla(const std::string& csv_path) {
    // Derivar nombre por defecto del nombre del archivo (sin extension)
    fs::path p(csv_path);
    std::string default_name = p.stem().string();

    std::cout << "Nombre para la tabla [" << default_name << "]: ";
    std::string name;
    std::getline(std::cin, name);

    // Quitar espacios
    while (!name.empty() && name.back() == ' ') name.pop_back();
    while (!name.empty() && name.front() == ' ') name.erase(name.begin());

    return name.empty() ? default_name : name;
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
    // 2. CATALOGO + CARGA INTERACTIVA DESDE CSV
    // ---------------------------------------------------------------
    PrintSectionTitle("2. Carga de datos desde CSV (interactivo)");

    Catalog catalog;
    catalog.SetDatabaseName("universidad");

    std::vector<HeapFile*> heap_files;  // guardar referencia a todos los heaps creados

    bool seguir_cargando = true;
    while (seguir_cargando) {
        std::string csv_path = ElegirCsv();
        if (csv_path.empty()) {
            std::cerr << "No se selecciono ningun CSV.\n";
            break;
        }

        // Verificar que el archivo existe
        if (!fs::exists(csv_path)) {
            std::cerr << "Error: el archivo '" << csv_path << "' no existe.\n";
            std::cout << "\nDesea intentar con otro CSV? (s/n): ";
            std::string resp;
            std::getline(std::cin, resp);
            if (resp == "s" || resp == "S" || resp == "si" || resp == "Si") continue;
            break;
        }

        std::string table_name = PedirNombreTabla(csv_path);

        HeapFile* heap = nullptr;
        bool loaded = CsvLoader::LoadCsvIntoTable(
            csv_path, table_name, disk_manager, catalog, &heap);

        if (!loaded || heap == nullptr) {
            std::cerr << "Fallo al cargar el CSV '" << csv_path << "'.\n";
        } else {
            heap_files.push_back(heap);
            std::cout << "\nRegistros insertados en la tabla '" << table_name << "':\n";
            heap->ScanAll();
        }

        std::cout << "\nDesea cargar otro CSV? (s/n): ";
        std::string resp;
        std::getline(std::cin, resp);
        if (resp != "s" && resp != "S" && resp != "si" && resp != "Si") {
            seguir_cargando = false;
        }
    }

    // Mostrar el catalogo completo
    PrintSectionTitle("Catalogo de la base de datos");
    catalog.Print();
    std::cout << "Paginas totales en el disco: " << disk_manager.GetNumPages() << "\n";

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
    // 4. PRUEBA DE INDICE HASH ESTATICO
    // ---------------------------------------------------------------
    PrintSectionTitle("4. Prueba de Indice Hash Estatico");

    // Buscaremos la tabla "alumnos" si fue cargada
    HeapFile* alumnos_hf_ptr = nullptr;
    for (auto* hf : heap_files) {
        // Obtenemos un registro para ver su estructura y adivinar si es alumnos
        char page_data[PAGE_SIZE];
        disk_manager.ReadPage(hf->GetFirstPageId(), page_data);
        Page p(hf->GetFirstPageId());
        std::memcpy(p.GetData(), page_data, PAGE_SIZE);
        if (p.GetRecordCount() > 0) {
            char rec_buf[PAGE_SIZE];
            int rec_size = 0;
            // Buscar un slot activo
            for (int s = 0; s < p.GetSlotCount(); s++) {
                if (p.GetRecord(s, rec_buf, rec_size)) {
                    Record r = Record::Deserialize(rec_buf, rec_size);
                    // Si el primer campo es INT y el segundo es STRING (como en alumnos), probamos
                    if (r.FieldCount() >= 2 && !r.IsNull(0) && !r.IsNull(1)) {
                        alumnos_hf_ptr = hf;
                        break;
                    }
                }
            }
        }
        if (alumnos_hf_ptr) break;
    }

    if (alumnos_hf_ptr) {
        std::cout << "Creando Indice Hash para la primera columna (ID) con 3 cubetas...\n";
        StaticHashIndex hash_index(disk_manager, 3); // 3 cubetas para forzar colisiones

        // Insertar registros en el indice
        const auto& p_ids = alumnos_hf_ptr->GetPageIds();
        for (int pid : p_ids) {
            char page_data[PAGE_SIZE];
            disk_manager.ReadPage(pid, page_data);
            Page page(pid);
            std::memcpy(page.GetData(), page_data, PAGE_SIZE);

            int slot_count = page.GetSlotCount();
            for (int slot_id = 0; slot_id < slot_count; slot_id++) {
                char record_buffer[PAGE_SIZE];
                int record_size = 0;
                if (page.GetRecord(slot_id, record_buffer, record_size)) {
                    Record r = Record::Deserialize(record_buffer, record_size);
                    if (!r.IsNull(0)) {
                        int key = r.GetInt(0);
                        RID rid{pid, slot_id};
                        hash_index.Insert(key, rid);
                    }
                }
            }
        }

        std::cout << "Indice creado en las paginas iniciales a partir de: " 
                  << hash_index.GetFirstPageId() << "\n";

        // Realizar una busqueda
        int search_key = 3;
        std::cout << "Buscando la clave " << search_key << " en el indice...\n";
        std::vector<RID> results;
        if (hash_index.Search(search_key, results)) {
            for (const auto& rid : results) {
                Record found_record;
                alumnos_hf_ptr->GetRecord(rid, found_record);
                std::cout << "  Encontrado en RID(" << rid.page_id << "," << rid.slot_id 
                          << ") -> " << found_record.ToString() << "\n";
            }
        } else {
            std::cout << "  Clave " << search_key << " no encontrada.\n";
        }
    } else {
        std::cout << "  (No se encontro tabla compatible cargada para probar el indice)\n";
    }

    // ---------------------------------------------------------------
    // 5. PRUEBA DE PERSISTENCIA: cerrar y volver a abrir el archivo
    // ---------------------------------------------------------------
    PrintSectionTitle("5. Prueba de persistencia (cerrar y reabrir el disco)");

    disk_manager.CloseDatabase();
    std::cout << "Disco cerrado.\n";

    DiskManager disk_manager_2(DB_NAME);
    if (!disk_manager_2.OpenDatabase()) {
        std::cerr << "Error al reabrir la base de datos.\n";
        return 1;
    }
    std::cout << "Disco reabierto. Paginas encontradas: "
              << disk_manager_2.GetNumPages() << "\n";

    // Reabrir cada tabla cargada y mostrar su contenido
    if (!heap_files.empty()) {
        for (auto* hf : heap_files) {
            HeapFile heap_reopen(disk_manager_2, hf->GetFirstPageId());
            std::cout << "\nContenido tras reabrir (primera pagina " << hf->GetFirstPageId() << "):\n";
            heap_reopen.ScanAll();
        }
    }

    // Limpiar memoria
    for (auto* hf : heap_files) {
        delete hf;
    }

    std::cout << "\nPrueba completa. El archivo '" << DB_NAME
              << "' quedo en el directorio actual.\n";
    std::cout << "Para inspeccionar el contenido: ./dump_db " << DB_NAME << "\n";

    return 0;
}
