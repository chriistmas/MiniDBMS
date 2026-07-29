#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <algorithm>
#include <cctype>
#include <filesystem>

#include "disk_manager.h"
#include "buffer_pool_manager.h"
#include "buffer_pool_adapter.h"
#include "heap_file.h"
#include "hash_index.h"
#include "bplus_tree.h"
#include "catalog.h"
#include "catalog_persistence.h"
#include "csv_loader.h"
#include "query_engine.h"

namespace fs = std::filesystem;

/*
 * MiniDBMS — capa de integracion final
 * -------------------------------------------------------------------------
 * Une los 4 modulos del sistema en un unico programa con un prompt
 * interactivo, en vez de 4 binarios de prueba independientes:
 *
 *   DiskManager  ->  BufferPoolManager  ->  Indices (B+/Hash)  ->  QueryEngine
 *
 * Al iniciar: crea o abre el archivo fisico .db, y si ya existia,
 * reconstruye el Catalog (leido de la Pagina 0) junto con un HeapFile por
 * cada tabla (con TODAS sus paginas, via HeapFile::RestorePageIds).
 *
 * Al salir: refresca la lista de paginas de cada tabla y persiste el
 * Catalog de vuelta a la Pagina 0, y hace flush de todo el Buffer Pool.
 * -------------------------------------------------------------------------
 */

namespace {

std::string ToUpper(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                    [](unsigned char c) { return std::toupper(c); });
    return out;
}

std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

void PrintBanner() {
    std::cout << "========================================================\n";
    std::cout << "  MiniDBMS -- Sistema Gestor de Base de Datos (Base de Datos II)\n";
    std::cout << "  Almacenamiento + Buffer Pool + Indices + Consultas (Volcano)\n";
    std::cout << "========================================================\n";
}

void PrintHelp() {
    std::cout <<
        "\nComandos disponibles:\n"
        "  \\dt                                   Listar tablas del catalogo\n"
        "  \\load <ruta.csv> <tabla>               Cargar un CSV como tabla nueva\n"
        "  \\index btree <tabla> <col> [orden]     Crear indice B+Tree (columna INTEGER)\n"
        "  \\index hash <tabla> <col> [cubetas]    Crear indice Hash (columna INTEGER)\n"
        "  \\stats                                 Estadisticas del Buffer Pool\n"
        "  \\help                                  Mostrar esta ayuda\n"
        "  \\quit / exit                           Guardar catalogo y salir\n"
        "\n"
        "  SELECT <col1,col2|*> FROM <tabla> [WHERE <col> <op> <valor>]\n"
        "      Operadores soportados: = != <> < <= > >=\n"
        "      Ejemplos:\n"
        "        SELECT * FROM alumnos\n"
        "        SELECT nombre, edad FROM alumnos WHERE edad > 20\n"
        "        SELECT * FROM alumnos WHERE id = 5\n";
}

// Handle de un indice construido en esta sesion (BPlusTree XOR StaticHashIndex).
struct IndexHandle {
    std::string type;   // "BTREE" | "HASH"
    std::string column;
    std::unique_ptr<BPlusTree> btree;
    std::unique_ptr<StaticHashIndex> hash;
};

int FindColumnIndex(const TableDefinition& table, const std::string& column_name) {
    for (size_t i = 0; i < table.columns.size(); i++) {
        if (table.columns[i].name == column_name) return static_cast<int>(i);
    }
    return -1;
}

} // namespace

int main(int argc, char** argv) {
    std::string db_name = (argc > 1) ? argv[1] : "minidbms.db";

    PrintBanner();

    // -----------------------------------------------------------------
    // 1. Abrir o crear el archivo fisico + Buffer Pool
    // -----------------------------------------------------------------
    DiskManager disk_manager(db_name);
    bool existed = disk_manager.DatabaseExists();
    bool opened = existed ? disk_manager.OpenDatabase() : disk_manager.CreateDatabase();
    if (!opened) {
        std::cerr << "No se pudo abrir/crear la base de datos '" << db_name << "'.\n";
        return 1;
    }

    BufferPoolManager bpm(disk_manager, /*pool_size=*/32);
    BufferPoolAdapter adapter(bpm); // TODO lo demas pasa por aqui, no por disk_manager directo

    // -----------------------------------------------------------------
    // 2. Cargar (o inicializar) el Catalog y reconstruir las tablas
    // -----------------------------------------------------------------
    Catalog catalog;
    std::map<std::string, HeapFile*> heap_files;

    if (existed && CatalogPersistence::Load(adapter, catalog)) {
        std::cout << "Base de datos '" << db_name << "' abierta. Catalogo restaurado: "
                  << catalog.GetTables().size() << " tabla(s).\n";
        for (auto& table : catalog.GetTables()) {
            HeapFile* hf = new HeapFile(adapter, table.first_page_id);
            hf->RestorePageIds(table.page_ids);
            heap_files[table.table_name] = hf;
        }
    } else {
        catalog.SetDatabaseName(fs::path(db_name).stem().string());
        std::cout << "Base de datos '" << db_name << "' "
                  << (existed ? "abierta (sin catalogo previo; se inicia vacio)." : "creada.")
                  << "\n";
    }

    QueryEngine engine(catalog);
    for (auto& [name, hf] : heap_files) {
        engine.RegisterTable(name, hf);
    }
    std::map<std::string, IndexHandle> indexes; // clave = "tabla.columna"

    PrintHelp();

    // -----------------------------------------------------------------
    // 3. Loop interactivo
    // -----------------------------------------------------------------
    std::string line;
    bool running = true;
    while (running) {
        std::cout << "\nminidbms> ";
        if (!std::getline(std::cin, line)) break;

        line = Trim(line);
        if (line.empty()) continue;
        if (!line.empty() && line.back() == ';') line.pop_back(); // tolerar ';' final

        std::istringstream iss(line);
        std::string command;
        iss >> command;
        std::string command_upper = ToUpper(command);

        if (command == "\\quit" || command == "\\q" || command_upper == "EXIT" ||
            command_upper == "QUIT") {
            running = false;

        } else if (command == "\\help" || command == "\\h") {
            PrintHelp();

        } else if (command == "\\dt") {
            catalog.Print();

        } else if (command == "\\load") {
            std::string csv_path, table_name;
            iss >> csv_path >> table_name;
            if (csv_path.empty() || table_name.empty()) {
                std::cout << "Uso: \\load <ruta.csv> <nombre_tabla>\n";
                continue;
            }
            if (!fs::exists(csv_path)) {
                std::cout << "Error: el archivo '" << csv_path << "' no existe.\n";
                continue;
            }
            if (heap_files.find(table_name) != heap_files.end()) {
                std::cout << "Error: ya existe una tabla '" << table_name << "' en esta sesion.\n";
                continue;
            }
            HeapFile* hf = nullptr;
            if (CsvLoader::LoadCsvIntoTable(csv_path, table_name, adapter, catalog, &hf)) {
                heap_files[table_name] = hf;
                engine.RegisterTable(table_name, hf);
                std::cout << "Tabla '" << table_name << "' cargada a traves del Buffer Pool ("
                          << hf->GetPageIds().size() << " pagina(s)).\n";
            } else {
                std::cout << "No se pudo cargar el CSV '" << csv_path << "'.\n";
            }

        } else if (command == "\\index") {
            std::string kind, table_name, column_name, extra;
            iss >> kind >> table_name >> column_name >> extra;
            std::string kind_upper = ToUpper(kind);

            auto hf_it = heap_files.find(table_name);
            if (hf_it == heap_files.end()) {
                std::cout << "Tabla no cargada en esta sesion: " << table_name << "\n";
                continue;
            }
            TableDefinition* table_def = catalog.FindTable(table_name);
            int col_idx = table_def ? FindColumnIndex(*table_def, column_name) : -1;
            if (col_idx == -1) {
                std::cout << "Columna no existe: " << column_name << "\n";
                continue;
            }
            if (table_def->columns[col_idx].type != "INTEGER") {
                std::cout << "Solo se pueden indexar columnas INTEGER (columna '"
                          << column_name << "' es " << table_def->columns[col_idx].type << ").\n";
                continue;
            }

            HeapFile* hf = hf_it->second;
            std::string key = table_name + "." + column_name;
            auto& handle = indexes[key];
            handle.column = column_name;

            if (kind_upper == "BTREE") {
                int order = extra.empty() ? 8 : std::stoi(extra);
                handle.type = "BTREE";
                handle.btree = std::make_unique<BPlusTree>(adapter, order);
                HeapFile::Iterator it = hf->GetIterator();
                it.Open();
                RID rid; Record rec;
                int count = 0;
                while (it.Next(rid, rec)) {
                    handle.btree->Insert(rec.GetInt(col_idx), rid);
                    count++;
                }
                BPlusTree* btree_ptr = handle.btree.get();
                engine.RegisterEqualityIndex(table_name, column_name,
                    [btree_ptr](int k, std::vector<RID>& out) { return btree_ptr->Search(k, out); });
                std::cout << "Indice B+Tree creado sobre " << key << " (" << count
                          << " claves, orden " << order << ").\n";

            } else if (kind_upper == "HASH") {
                int num_buckets = extra.empty() ? 8 : std::stoi(extra);
                handle.type = "HASH";
                handle.hash = std::make_unique<StaticHashIndex>(adapter, num_buckets);
                HeapFile::Iterator it = hf->GetIterator();
                it.Open();
                RID rid; Record rec;
                int count = 0;
                while (it.Next(rid, rec)) {
                    handle.hash->Insert(rec.GetInt(col_idx), rid);
                    count++;
                }
                StaticHashIndex* hash_ptr = handle.hash.get();
                engine.RegisterEqualityIndex(table_name, column_name,
                    [hash_ptr](int k, std::vector<RID>& out) { return hash_ptr->Search(k, out); });
                std::cout << "Indice Hash creado sobre " << key << " (" << count
                          << " claves, " << num_buckets << " cubetas).\n";

            } else {
                indexes.erase(key);
                std::cout << "Uso: \\index btree|hash <tabla> <columna> [orden|cubetas]\n";
            }

        } else if (command == "\\stats") {
            std::cout << "Pool size: " << bpm.GetPoolSize() << "\n";
            std::cout << "Hits: " << bpm.GetHitCount() << "  Misses: " << bpm.GetMissCount() << "\n";
            std::cout << "Paginas totales en disco: " << bpm.GetNumPages() << "\n";
            std::cout << "Tablas cargadas: " << heap_files.size()
                       << "  |  Indices activos: " << indexes.size() << "\n";

        } else if (command_upper == "SELECT") {
            engine.Execute(line);

        } else {
            std::cout << "Comando no reconocido: '" << command
                      << "'. Escriba \\help para ver los comandos disponibles.\n";
        }
    }

    // -----------------------------------------------------------------
    // 4. Persistir catalogo (con la lista de paginas actualizada) y cerrar
    // -----------------------------------------------------------------
    for (auto& table : catalog.GetTables()) {
        auto it = heap_files.find(table.table_name);
        if (it != heap_files.end()) {
            table.page_ids = it->second->GetPageIds();
        }
    }
    bool saved = CatalogPersistence::Save(adapter, catalog);
    bpm.FlushAllPages();
    disk_manager.CloseDatabase();

    std::cout << "\nCatalogo " << (saved ? "guardado en la Pagina 0." : "NO pudo guardarse (excede 1 pagina).")
              << "\n";
    std::cout << "Buffer Pool -> Hits: " << bpm.GetHitCount() << "  Misses: " << bpm.GetMissCount() << "\n";
    std::cout << "Base de datos cerrada: " << db_name << "\n";

    for (auto& [name, hf] : heap_files) delete hf;
    return 0;
}
