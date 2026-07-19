/*
 * dump_db — Herramienta de inspeccion de archivos .db del MiniDBMS
 * ----------------------------------------------------------------
 * Lee un archivo .db generado por el DiskManager y muestra su contenido
 * de forma legible: paginas, slots, registros, espacio libre, etc.
 *
 * Uso:
 *   ./dump_db <archivo.db>
 *
 * Ejemplo:
 *   ./dump_db universidad.db
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <sstream>
#include "page.h"
#include "page_header.h"
#include "record.h"

// ---------------------------------------------------------------
// Imprime un volcado hexadecimal de los primeros N bytes de data
// ---------------------------------------------------------------
void PrintHexDump(const char* data, int length, int max_bytes = 64) {
    int limit = std::min(length, max_bytes);
    for (int i = 0; i < limit; i++) {
        if (i > 0 && i % 16 == 0) std::cout << "\n";
        std::cout << std::hex << std::setfill('0') << std::setw(2)
                  << (static_cast<unsigned int>(static_cast<unsigned char>(data[i])))
                  << " ";
    }
    std::cout << std::dec << "\n";
    if (limit < length) {
        std::cout << "  ... (" << (length - limit) << " bytes mas)\n";
    }
}

// ---------------------------------------------------------------
// Intenta deserializar y mostrar un registro
// ---------------------------------------------------------------
void TryPrintRecord(const char* buffer, int size) {
    try {
        Record r = Record::Deserialize(buffer, size);
        std::cout << r.ToString() << "\n";
    } catch (...) {
        std::cout << "(error al deserializar)\n";
        std::cout << "        Hex: ";
        PrintHexDump(buffer, size, 32);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <archivo.db>\n";
        std::cerr << "\nEjemplo:\n";
        std::cerr << "  " << argv[0] << " universidad.db\n";
        return 1;
    }

    const char* filename = argv[1];

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: no se pudo abrir '" << filename << "'\n";
        return 1;
    }

    long long file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    int num_pages = static_cast<int>(file_size / PAGE_SIZE);
    int remainder = static_cast<int>(file_size % PAGE_SIZE);

    std::cout << "================================================================\n";
    std::cout << "  dump_db — Inspeccion de archivo: " << filename << "\n";
    std::cout << "================================================================\n\n";

    std::cout << "Tamano del archivo:  " << file_size << " bytes\n";
    std::cout << "Tamano de pagina:    " << PAGE_SIZE << " bytes\n";
    std::cout << "Numero de paginas:   " << num_pages << "\n";
    if (remainder > 0) {
        std::cout << "** ADVERTENCIA: el archivo tiene " << remainder
                  << " bytes extra que no forman una pagina completa **\n";
    }
    std::cout << "\n";

    // Leer y mostrar cada pagina
    for (int p = 0; p < num_pages; p++) {
        char page_data[PAGE_SIZE];
        file.read(page_data, PAGE_SIZE);

        // Interpretar el header
        const PageHeader* header = reinterpret_cast<const PageHeader*>(page_data);

        std::cout << "----------------------------------------------------------------\n";
        std::cout << "  Pagina " << p << "\n";
        std::cout << "----------------------------------------------------------------\n";
        std::cout << "  Page ID:              " << header->page_id << "\n";
        std::cout << "  Next Page ID:         " << header->next_page_id << "\n";
        std::cout << "  Record Count:         " << header->record_count << "\n";
        std::cout << "  Slot Count:           " << header->slot_count << "\n";
        std::cout << "  Free Space:           " << header->free_space << " bytes\n";
        std::cout << "  Free Space Pointer:   " << header->free_space_pointer << "\n";

        // Validar que el header tenga valores razonables
        bool header_valido = (header->page_id >= 0 && header->page_id < 100000 &&
                              header->slot_count >= 0 && header->slot_count < 10000 &&
                              header->record_count >= 0 &&
                              header->record_count <= header->slot_count &&
                              header->free_space >= 0 && header->free_space <= PAGE_SIZE &&
                              header->free_space_pointer >= 0 &&
                              header->free_space_pointer <= PAGE_SIZE);

        if (!header_valido) {
            std::cout << "\n  ** Header con valores sospechosos (pagina posiblemente vacia o corrupta) **\n";
            std::cout << "  Primeros 64 bytes (hex):\n  ";
            PrintHexDump(page_data, PAGE_SIZE, 64);
            std::cout << "\n";
            continue;
        }

        if (header->slot_count == 0) {
            std::cout << "\n  (Pagina vacia — sin registros)\n\n";
            continue;
        }

        // Leer el Slot Directory
        const Slot* slots = reinterpret_cast<const Slot*>(page_data + sizeof(PageHeader));

        std::cout << "\n  Slot Directory:\n";
        std::cout << "  " << std::setw(8) << "Slot"
                  << std::setw(12) << "Offset"
                  << std::setw(12) << "Length"
                  << std::setw(12) << "Estado" << "\n";
        std::cout << "  " << std::string(44, '-') << "\n";

        for (int s = 0; s < header->slot_count; s++) {
            std::cout << "  " << std::setw(8) << s
                      << std::setw(12) << slots[s].offset
                      << std::setw(12) << slots[s].length
                      << std::setw(12) << (slots[s].offset == -1 ? "BORRADO" : "ACTIVO")
                      << "\n";
        }

        // Mostrar los registros activos
        std::cout << "\n  Registros:\n";
        int registros_mostrados = 0;
        for (int s = 0; s < header->slot_count; s++) {
            if (slots[s].offset == -1) continue;

            // Validar que el offset y length son razonables
            if (slots[s].offset < 0 || slots[s].offset >= PAGE_SIZE ||
                slots[s].length <= 0 || slots[s].offset + slots[s].length > PAGE_SIZE) {
                std::cout << "    Slot " << s << ": (offset/length invalido)\n";
                continue;
            }

            std::cout << "    Slot " << s << " [offset=" << slots[s].offset
                      << ", length=" << slots[s].length << "]: ";

            TryPrintRecord(page_data + slots[s].offset, slots[s].length);
            registros_mostrados++;
        }

        if (registros_mostrados == 0) {
            std::cout << "    (ninguno activo)\n";
        }

        std::cout << "\n";
    }

    file.close();

    std::cout << "================================================================\n";
    std::cout << "  Fin del volcado. Total: " << num_pages << " paginas.\n";
    std::cout << "================================================================\n";

    return 0;
}
