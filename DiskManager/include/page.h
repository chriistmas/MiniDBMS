#ifndef PAGE_H
#define PAGE_H

#include "page_header.h"

// Tamano de pagina del DBMS (unidad logica de lectura/escritura)
const int PAGE_SIZE = 4096;

/*
 * Slot
 * -------------------------------------------------------------------------
 * Entrada del "Slot Directory". Cada slot indica donde (offset) y que
 * tan largo (length) es un registro dentro del area de datos de la pagina.
 * offset == -1 indica que el slot fue eliminado (tombstone).
 * -------------------------------------------------------------------------
 */
struct Slot {
    int offset;
    int length;
};

/*
 * Page
 * -------------------------------------------------------------------------
 * Representa una pagina del DBMS organizada como "Slotted Page":
 *
 *   +----------------------+
 *   |     Page Header      |
 *   +----------------------+
 *   |    Slot Directory     |  -> crece hacia adelante
 *   +----------------------+
 *   |                      |
 *   |   Registros/Tuplas   |  -> crecen hacia atras (desde el final)
 *   |                      |
 *   +----------------------+
 *   |     Espacio libre     |
 *   +----------------------+
 *
 * La pagina se llena cuando el Slot Directory y el area de datos se
 * encuentran en el medio.
 * -------------------------------------------------------------------------
 */
class Page {
public:
    Page();
    explicit Page(int page_id);

    char* GetData();
    const char* GetData() const;

    int GetPageId() const;
    void SetPageId(int page_id);

    int GetNextPageId() const;
    void SetNextPageId(int next_page_id);

    // Inserta un registro serializado. Devuelve el slot_id o -1 si no cabe.
    int InsertRecord(const char* record_data, int record_size);

    // Obtiene el registro almacenado en un slot. Devuelve false si no existe.
    bool GetRecord(int slot_id, char* out_buffer, int& out_size) const;

    // Elimina (logicamente) el registro de un slot.
    bool DeleteRecord(int slot_id);

    // Actualiza el registro de un slot. Puede reubicarlo dentro de la misma pagina.
    bool UpdateRecord(int slot_id, const char* record_data, int record_size);

    int GetRecordCount() const;
    int GetSlotCount() const;
    int GetFreeSpace() const;

private:
    char data_[PAGE_SIZE];

    PageHeader* Header();
    const PageHeader* Header() const;
    Slot* SlotArray();
    const Slot* SlotArray() const;

    void InitEmptyPage(int page_id);
};

#endif // PAGE_H
