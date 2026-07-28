#include <iostream>
#include "disk_manager.h"
#include "buffer_pool_manager.h"
#include "buffer_pool_adapter.h"
#include "heap_file.h"
#include "hash_index.h"
#include "bplus_tree.h"
#include "catalog.h"
#include "query_engine.h"

static void Section(const std::string& title) {
    std::cout << "\n########################################################\n";
    std::cout << "# " << title << "\n";
    std::cout << "########################################################\n";
}

int main() {
    Section("1. Disco + Buffer Pool + tabla 'alumnos' (id INTEGER, nombre STRING, edad INTEGER)");
    DiskManager disk_manager("query_engine_test.db");
    disk_manager.CreateDatabase();

    BufferPoolManager bpm(disk_manager, /*pool_size=*/16);
    BufferPoolAdapter adapter(bpm);

    Catalog catalog;
    catalog.SetDatabaseName("universidad");

    int first_page = adapter.AllocatePage();
    TableDefinition table_def;
    table_def.table_name = "alumnos";
    table_def.columns = {{"id", "INTEGER"}, {"nombre", "STRING"}, {"edad", "INTEGER"}};
    table_def.first_page_id = first_page;
    catalog.AddTable(table_def);

    HeapFile alumnos(adapter, first_page);

    struct Row { int id; std::string nombre; int edad; };
    std::vector<Row> data = {
        {1, "Ana", 20}, {2, "Luis", 22}, {3, "Marta", 19}, {4, "Pedro", 25},
        {5, "Sofia", 21}, {6, "Diego", 23}, {7, "Valeria", 18}, {8, "Jorge", 24},
    };
    for (const auto& row : data) {
        Record r;
        r.AddInt(row.id);
        r.AddString(row.nombre);
        r.AddInt(row.edad);
        alumnos.InsertRecord(r);
    }
    std::cout << "Insertados " << data.size() << " alumnos.\n";

    Section("2. Construir un B+Tree sobre la columna 'id' (usando el Buffer Pool)");
    BPlusTree id_index(adapter, /*max_entries=*/4);
    {
        HeapFile::Iterator it = alumnos.GetIterator();
        it.Open();
        RID rid;
        Record rec;
        while (it.Next(rid, rec)) {
            id_index.Insert(rec.GetInt(0), rid);
        }
    }

    Section("3. Registrar la tabla y el indice en el QueryEngine");
    QueryEngine engine(catalog);
    engine.RegisterTable("alumnos", &alumnos);
    engine.RegisterEqualityIndex("alumnos", "id",
        [&id_index](int key, std::vector<RID>& out) { return id_index.Search(key, out); });

    Section("4. SELECT * FROM alumnos  (SeqScanOperator -> ProjectOperator)");
    engine.Execute("SELECT * FROM alumnos");

    Section("5. SELECT nombre, edad FROM alumnos WHERE edad > 20  (SeqScan -> Filter -> Project)");
    engine.Execute("SELECT nombre, edad FROM alumnos WHERE edad > 20");

    Section("6. SELECT * FROM alumnos WHERE id = 5  (usa el B+Tree -> IndexScanOperator)");
    engine.Execute("SELECT * FROM alumnos WHERE id = 5");

    Section("7. SELECT nombre FROM alumnos WHERE nombre = 'Diego'  (columna sin indice -> SeqScan+Filter)");
    engine.Execute("SELECT nombre FROM alumnos WHERE nombre = 'Diego'");

    Section("8. Casos de error controlados por el Parser / Planner");
    engine.Execute("SELECT * FROM tabla_inexistente");
    engine.Execute("SELECT columna_rara FROM alumnos");
    engine.Execute("SELECCIONAR * FROM alumnos"); // palabra clave invalida

    bpm.FlushAllPages();
    std::cout << "\nHits: " << bpm.GetHitCount() << "  Misses: " << bpm.GetMissCount() << "\n";
    disk_manager.CloseDatabase();
    std::cout << "\nPrueba del motor de consultas finalizada correctamente.\n";
    return 0;
}
