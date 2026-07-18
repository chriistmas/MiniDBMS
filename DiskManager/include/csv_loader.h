#ifndef CSV_LOADER_H
#define CSV_LOADER_H

#include <string>
#include "disk_manager.h"
#include "catalog.h"
#include "heap_file.h"

/*
 * CsvLoader
 * -------------------------------------------------------------------------
 * Implementa el flujo:
 *
 *   CSV -> Parser CSV -> Crear esquema -> Crear archivo/paginas fisicas
 *        -> Convertir filas en registros -> Guardar registros en paginas
 *
 * A partir de un archivo CSV con encabezado, infiere el esquema (columna
 * y tipo INTEGER/STRING), crea la tabla en el catalogo y crea un HeapFile
 * donde inserta cada fila como un Record.
 * -------------------------------------------------------------------------
 */
class CsvLoader {
public:
    // Carga un CSV hacia una nueva tabla. Devuelve el HeapFile creado
    // (el llamador es responsable de conservarlo mientras use la tabla).
    static bool LoadCsvIntoTable(const std::string& csv_path,
                                  const std::string& table_name,
                                  DiskManager& disk_manager,
                                  Catalog& catalog,
                                  HeapFile** out_heap_file);

private:
    static std::vector<std::string> SplitCsvLine(const std::string& line);
    static bool LooksLikeInteger(const std::string& token);
};

#endif // CSV_LOADER_H
