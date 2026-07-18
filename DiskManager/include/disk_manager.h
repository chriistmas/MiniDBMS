#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include <string>
#include <fstream>
#include <vector>
#include "page.h"
#include "physical_disk.h"

/*
 * DiskManager
 * -------------------------------------------------------------------------
 * Responsable de interactuar directamente con el sistema operativo (Ubuntu)
 * y administrar el archivo fisico (.db) donde se almacena la base de datos.
 *
 * Jerarquia que administra:
 *   Disco fisico (simulado con PhysicalDisk)
 *        -> Archivo de base de datos (.db)
 *              -> Paginas (PAGE_SIZE bytes cada una)
 *
 * Responsabilidades:
 *   - Crear y abrir el archivo binario de la base de datos.
 *   - Leer y escribir paginas usando: offset = Page_ID * PAGE_SIZE
 *   - Asignar (AllocatePage) y liberar (DeallocatePage) paginas.
 *   - Administrar que paginas existen, cuales estan libres y cuales ocupadas.
 *
 * NO administra (responsabilidad del Buffer Manager, fuera de este modulo):
 *   - Buffer Pool, Frames, LRU/FIFO/Clock, Dirty Pages, Pin/Unpin, RAM.
 * -------------------------------------------------------------------------
 */
class DiskManager {
public:
    explicit DiskManager(const std::string& db_filename);
    ~DiskManager();

    // Crea el archivo fisico de la base de datos desde cero (simulando el disco).
    // Reserva automaticamente la Pagina 0 para el catalogo.
    bool CreateDatabase();

    // Abre un archivo de base de datos ya existente.
    bool OpenDatabase();

    void CloseDatabase();

    // Lee PAGE_SIZE bytes desde el archivo hacia page_data (buffer ya reservado).
    void ReadPage(int page_id, char* page_data);

    // Escribe PAGE_SIZE bytes desde page_data hacia el archivo.
    void WritePage(int page_id, const char* page_data);

    // Asigna una nueva pagina (reutiliza una libre si existe) y devuelve su Page ID.
    int AllocatePage();

    // Marca una pagina como libre para ser reutilizada mas adelante.
    void DeallocatePage(int page_id);

    int GetNumPages() const;
    bool DatabaseExists() const;
    const std::string& GetFilename() const;

    // Expone la simulacion de la geometria fisica (superficies/pistas/sectores)
    const PhysicalDisk& GetPhysicalDisk() const;

private:
    std::string db_filename_;
    std::fstream db_file_;
    int num_pages_;
    std::vector<bool> free_pages_; // true = pagina libre / reutilizable

    PhysicalDisk physical_disk_;   // simulacion de la geometria fisica del disco

    long long CalculateOffset(int page_id) const;
};

#endif // DISK_MANAGER_H
